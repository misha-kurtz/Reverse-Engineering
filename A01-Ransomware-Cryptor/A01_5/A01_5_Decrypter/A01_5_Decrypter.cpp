// A01_5 Decrypter for controlled double-extortion ransomware sample
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <windows.h>
#include <wincrypt.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "crypt32.lib")

namespace fs = std::filesystem;

class A01_5_Decrypter
{
private:
    std::string aes_key;
    std::string userName;
    std::string computerName;
    std::string userDir = "C:\\Users\\";

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
    A01_5_Decrypter()
    {
        userName = get_env_var("USERNAME");
        computerName = get_computer_name();
    }

    void startDecryption()
    {
        std::string dataPath = "C:\\Users\\Public\\A01_TestData";

        std::string keyPath = "C:\\Users\\Public\\A01_5_Lab_Encryption_Key.txt";

        std::ifstream keyFile(keyPath);
        if (!keyFile)
            return;

        std::getline(keyFile, aes_key);
        keyFile.close();

        if (fs::exists(dataPath) && fs::is_directory(dataPath))
        {
            DecryptDirectory(dataPath, aes_key);
        }

    }

    void DecryptFile(const std::string &file, const std::string &key)
    {
        const std::string lockedExt = ".locked";

        if (file.length() < lockedExt.length() ||
            file.compare(file.length() - lockedExt.length(), lockedExt.length(), lockedExt) != 0)
        {
            return;
        }

        std::ifstream input(file, std::ios::binary);
        if (!input)
            return;

        std::vector<unsigned char> encryptedBytes(
            (std::istreambuf_iterator<char>(input)),
            std::istreambuf_iterator<char>());

        input.close();

        std::vector<unsigned char> decryptedBytes = AES_Decrypt(encryptedBytes, key);

        if (decryptedBytes.empty())
            return;

        std::ofstream output(file, std::ios::binary | std::ios::trunc);
        if (!output)
            return;

        output.write(
            reinterpret_cast<const char *>(decryptedBytes.data()),
            decryptedBytes.size());

        output.close();

        std::string restoredPath = file.substr(0, file.length() - 7);
        std::filesystem::rename(file, restoredPath);
    }

    void DecryptDirectory(
        const std::string &location,
        const std::string &key)
    {
        try
        {
            for (const auto &entry : fs::directory_iterator(location))
            {
                if (fs::is_regular_file(entry.path()))
                {
                    std::string filename = entry.path().string();

                    if (entry.path().extension() == ".locked")
                    {
                        try
                        {
                            DecryptFile(filename, key);
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
                        DecryptDirectory(entry.path().string(), key);
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

    std::vector<unsigned char> AES_Decrypt(
        const std::vector<unsigned char> &encryptedBytes,
        const std::string &keyMaterial)
    {
        std::vector<unsigned char> decrypted = encryptedBytes;

        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        HCRYPTKEY hKey = 0;

        if (!CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
            return {};

        if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
        {
            CryptReleaseContext(hProv, 0);
            return {};
        }

        if (!CryptHashData(
                hHash,
                reinterpret_cast<const BYTE *>(keyMaterial.data()),
                static_cast<DWORD>(keyMaterial.size()),
                0))
        {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return {};
        }

        if (!CryptDeriveKey(hProv, CALG_AES_256, hHash, 0, &hKey))
        {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return {};
        }

        DWORD mode = CRYPT_MODE_CBC;
        CryptSetKeyParam(hKey, KP_MODE, reinterpret_cast<BYTE *>(&mode), 0);

        BYTE iv[16] = {0};
        CryptSetKeyParam(hKey, KP_IV, iv, 0);

        DWORD dataLen = static_cast<DWORD>(decrypted.size());

        if (!CryptDecrypt(hKey, 0, TRUE, 0, decrypted.data(), &dataLen))
        {
            decrypted.clear();
        }
        else
        {
            decrypted.resize(dataLen);
        }

        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);

        return decrypted;
    }

    

    
};

int main()
{
    A01_5_Decrypter app;
    app.startDecryption();
    return 0;
}

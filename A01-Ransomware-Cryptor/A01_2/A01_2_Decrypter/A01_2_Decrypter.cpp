#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstdlib>
#include <iterator>

#include <windows.h>
#include <wincrypt.h>

#pragma comment(lib, "advapi32.lib")

namespace fs = std::filesystem;

namespace a01_2_decrypter
{
    const std::string KEY_PATH = "C:\\Users\\Public\\A01_2_Lab_Encryption_Key.txt";
    const std::string TARGET_DIR = "C:\\Users\\Public\\A01_TestData";

    std::string read_key()
    {
        std::ifstream key_file(KEY_PATH);
        std::string key;

        if (key_file)
        {
            std::getline(key_file, key);
        }

        return key;
    }

    std::vector<unsigned char> aes_decrypt(
        const std::vector<unsigned char> &ciphertext,
        const std::string &key)
    {
        std::vector<unsigned char> decrypted = ciphertext;

        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        HCRYPTKEY hKey = 0;

        if (!CryptAcquireContext(
                &hProv,
                nullptr,
                nullptr,
                PROV_RSA_AES,
                CRYPT_VERIFYCONTEXT))
        {
            return {};
        }

        if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
        {
            CryptReleaseContext(hProv, 0);
            return {};
        }

        if (!CryptHashData(
                hHash,
                reinterpret_cast<const BYTE *>(key.data()),
                static_cast<DWORD>(key.size()),
                0))
        {
            CryptDestroyHash(hHash);
            CryptReleaseContext(hProv, 0);
            return {};
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
            return {};
        }

        DWORD dataLen = static_cast<DWORD>(decrypted.size());

        if (!CryptDecrypt(
                hKey,
                0,
                TRUE,
                0,
                decrypted.data(),
                &dataLen))
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

    void decrypt_file(const std::string &file, const std::string &key)
    {
        fs::path file_path(file);

        if (file_path.extension().string() != ".locked")
        {
            return;
        }

        std::ifstream input(file, std::ios::binary);
        if (!input)
        {
            return;
        }

        std::vector<unsigned char> bytes_to_decrypt(
            (std::istreambuf_iterator<char>(input)),
            std::istreambuf_iterator<char>());
        input.close();

        std::vector<unsigned char> bytes_decrypted =
            aes_decrypt(bytes_to_decrypt, key);

        if (bytes_decrypted.empty())
        {
            return;
        }

        std::ofstream output(file, std::ios::binary | std::ios::trunc);
        if (!output)
        {
            return;
        }

        output.write(
            reinterpret_cast<const char *>(bytes_decrypted.data()),
            bytes_decrypted.size());
        output.close();

        fs::path restored_path = file_path;
        restored_path.replace_extension("");

        fs::rename(file_path, restored_path);
    }

    void decrypt_directory(const std::string &location, const std::string &key)
    {
        try
        {
            for (const auto &entry : fs::directory_iterator(location))
            {
                try
                {
                    if (fs::is_regular_file(entry.path()))
                    {
                        decrypt_file(entry.path().string(), key);
                    }
                    else if (fs::is_directory(entry.path()))
                    {
                        decrypt_directory(entry.path().string(), key);
                    }
                }
                catch (...)
                {
                    // Ignore individual file or directory errors.
                }
            }
        }
        catch (...)
        {
            // Ignore inaccessible root directory errors.
        }
    }
}

int main()
{
    std::string key = a01_2_decrypter::read_key();

    if (key.empty())
    {
        return 1;
    }

    if (fs::exists(a01_2_decrypter::TARGET_DIR) &&
        fs::is_directory(a01_2_decrypter::TARGET_DIR))
    {
        a01_2_decrypter::decrypt_directory(
            a01_2_decrypter::TARGET_DIR,
            key);
    }

    return 0;
}
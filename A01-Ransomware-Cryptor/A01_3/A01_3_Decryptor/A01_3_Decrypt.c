// A01_3 decrypter for the controlled Hidden-Tear recovery-inhibition sample

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include <wincrypt.h>
#include <shlwapi.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shlwapi.lib")

#define KEY_PATH \
    "C:\\Users\\Public\\A01_3_Lab_Encryption_Key.txt"

#define PRIMARY_TARGET \
    "C:\\Users\\Public\\A01_TestData"

#define KEY_BUFFER_SIZE 256

// Function prototypes
BOOL read_key(
    const char *key_path,
    char *key_buffer,
    size_t key_buffer_size);

unsigned char *aes_decrypt(
    const unsigned char *ciphertext,
    size_t ciphertext_len,
    const char *key,
    size_t *out_len);

BOOL decrypt_file(
    const char *file,
    const char *key);

void decrypt_directory(
    const char *location,
    const char *key);

const char *EnumBackupVolume(void);

BOOL InspectVolume(
    const char *driveLetter);

void to_lower_string(char *str);

// Converts a string to lowercase
void to_lower_string(char *str)
{
    for (; *str; ++str)
    {
        *str = (char)tolower((unsigned char)*str);
    }
}

// Reads the saved laboratory encryption key
BOOL read_key(
    const char *key_path,
    char *key_buffer,
    size_t key_buffer_size)
{
    if (key_path == NULL ||
        key_buffer == NULL ||
        key_buffer_size == 0)
    {
        return FALSE;
    }

    FILE *key_file = fopen(key_path, "r");

    if (key_file == NULL)
    {
        return FALSE;
    }

    if (fgets(
            key_buffer,
            (int)key_buffer_size,
            key_file) == NULL)
    {
        fclose(key_file);
        return FALSE;
    }

    fclose(key_file);

    /*
     * Remove any newline characters that may be present
     * in the key file.
     */
    key_buffer[strcspn(key_buffer, "\r\n")] = '\0';

    return key_buffer[0] != '\0';
}

// Decrypts a byte buffer using the same CryptoAPI derivation
// used by the A01_3 encryptor
unsigned char *aes_decrypt(
    const unsigned char *ciphertext,
    size_t ciphertext_len,
    const char *key,
    size_t *out_len)
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HCRYPTKEY hKey = 0;

    unsigned char *decrypted = NULL;

    if (ciphertext == NULL ||
        key == NULL ||
        out_len == NULL ||
        ciphertext_len == 0 ||
        ciphertext_len > MAXDWORD)
    {
        return NULL;
    }

    *out_len = 0;

    if (!CryptAcquireContextA(
            &hProv,
            NULL,
            NULL,
            PROV_RSA_AES,
            CRYPT_VERIFYCONTEXT))
    {
        return NULL;
    }

    if (!CryptCreateHash(
            hProv,
            CALG_SHA_256,
            0,
            0,
            &hHash))
    {
        CryptReleaseContext(hProv, 0);
        return NULL;
    }

    if (!CryptHashData(
            hHash,
            (const BYTE *)key,
            (DWORD)strlen(key),
            0))
    {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return NULL;
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
        return NULL;
    }

    decrypted =
        (unsigned char *)malloc(ciphertext_len);

    if (decrypted == NULL)
    {
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return NULL;
    }

    memcpy(
        decrypted,
        ciphertext,
        ciphertext_len);

    DWORD data_len =
        (DWORD)ciphertext_len;

    if (!CryptDecrypt(
            hKey,
            0,
            TRUE,
            0,
            decrypted,
            &data_len))
    {
        free(decrypted);
        decrypted = NULL;
    }
    else
    {
        *out_len = (size_t)data_len;
    }

    CryptDestroyKey(hKey);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    return decrypted;
}

// Decrypts one .locked file and restores its original name
BOOL decrypt_file(
    const char *file,
    const char *key)
{
    if (file == NULL || key == NULL)
    {
        return FALSE;
    }

    const char *extension =
        PathFindExtensionA(file);

    char ext_lower[MAX_PATH];

    strncpy(
        ext_lower,
        extension,
        MAX_PATH - 1);

    ext_lower[MAX_PATH - 1] = '\0';

    to_lower_string(ext_lower);

    if (strcmp(ext_lower, ".locked") != 0)
    {
        return FALSE;
    }

    FILE *input = fopen(file, "rb");

    if (input == NULL)
    {
        return FALSE;
    }

    if (fseek(input, 0, SEEK_END) != 0)
    {
        fclose(input);
        return FALSE;
    }

    long file_size = ftell(input);

    if (file_size <= 0)
    {
        fclose(input);
        return FALSE;
    }

    if (fseek(input, 0, SEEK_SET) != 0)
    {
        fclose(input);
        return FALSE;
    }

    unsigned char *ciphertext =
        (unsigned char *)malloc((size_t)file_size);

    if (ciphertext == NULL)
    {
        fclose(input);
        return FALSE;
    }

    size_t bytes_read = fread(
        ciphertext,
        1,
        (size_t)file_size,
        input);

    fclose(input);

    if (bytes_read != (size_t)file_size)
    {
        free(ciphertext);
        return FALSE;
    }

    size_t decrypted_len = 0;

    unsigned char *decrypted =
        aes_decrypt(
            ciphertext,
            (size_t)file_size,
            key,
            &decrypted_len);

    free(ciphertext);

    if (decrypted == NULL)
    {
        return FALSE;
    }

    FILE *output = fopen(file, "wb");

    if (output == NULL)
    {
        free(decrypted);
        return FALSE;
    }

    size_t bytes_written = fwrite(
        decrypted,
        1,
        decrypted_len,
        output);

    fclose(output);

    SecureZeroMemory(
        decrypted,
        decrypted_len);

    free(decrypted);

    if (bytes_written != decrypted_len)
    {
        return FALSE;
    }

    /*
     * Remove the final ".locked" extension.
     *
     * Example:
     * report.docx.locked -> report.docx
     */
    size_t file_len = strlen(file);
    size_t extension_len = strlen(".locked");

    if (file_len <= extension_len)
    {
        return FALSE;
    }

    char restored_name[MAX_PATH];

    int result = snprintf(
        restored_name,
        sizeof(restored_name),
        "%.*s",
        (int)(file_len - extension_len),
        file);

    if (result < 0 ||
        result >= (int)sizeof(restored_name))
    {
        return FALSE;
    }

    if (!MoveFileExA(
            file,
            restored_name,
            MOVEFILE_REPLACE_EXISTING))
    {
        return FALSE;
    }

    return TRUE;
}

// Recursively decrypts .locked files
void decrypt_directory(
    const char *location,
    const char *key)
{
    if (location == NULL || key == NULL)
    {
        return;
    }

    char search_path[MAX_PATH];

    int search_result = snprintf(
        search_path,
        sizeof(search_path),
        "%s\\*",
        location);

    if (search_result < 0 ||
        search_result >= (int)sizeof(search_path))
    {
        return;
    }

    WIN32_FIND_DATAA find_data;

    HANDLE hFind =
        FindFirstFileA(
            search_path,
            &find_data);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        return;
    }

    do
    {
        if (strcmp(find_data.cFileName, ".") == 0 ||
            strcmp(find_data.cFileName, "..") == 0)
        {
            continue;
        }

        char full_path[MAX_PATH];

        int path_result = snprintf(
            full_path,
            sizeof(full_path),
            "%s\\%s",
            location,
            find_data.cFileName);

        if (path_result < 0 ||
            path_result >= (int)sizeof(full_path))
        {
            continue;
        }

        /*
         * Avoid following junctions and symbolic links.
         */
        if (find_data.dwFileAttributes &
            FILE_ATTRIBUTE_REPARSE_POINT)
        {
            continue;
        }

        if (find_data.dwFileAttributes &
            FILE_ATTRIBUTE_DIRECTORY)
        {
            decrypt_directory(
                full_path,
                key);
        }
        else
        {
            decrypt_file(
                full_path,
                key);
        }

    } while (FindNextFileA(
        hFind,
        &find_data));

    FindClose(hFind);
}

// Enumerates logical volumes and returns the volume labeled Backup
const char *EnumBackupVolume(void)
{
    static char backupDrive[MAX_PATH] = {0};

    char driveBuffer[256] = {0};

    DWORD bytesReturned =
        GetLogicalDriveStringsA(
            sizeof(driveBuffer) - 1,
            driveBuffer);

    if (bytesReturned == 0 ||
        bytesReturned >= sizeof(driveBuffer))
    {
        return NULL;
    }

    char *driveLetter = driveBuffer;

    while (*driveLetter)
    {
        UINT driveType =
            GetDriveTypeA(driveLetter);

        if (driveType == DRIVE_FIXED ||
            driveType == DRIVE_REMOTE)
        {
            if (InspectVolume(driveLetter))
            {
                strncpy(
                    backupDrive,
                    driveLetter,
                    MAX_PATH - 1);

                backupDrive[MAX_PATH - 1] = '\0';

                return backupDrive;
            }
        }

        driveLetter +=
            strlen(driveLetter) + 1;
    }

    return NULL;
}

// Checks whether a volume has the controlled Backup label
BOOL InspectVolume(const char *driveLetter)
{
    char volumeName[MAX_PATH] = {0};
    char fileSystemName[MAX_PATH] = {0};

    DWORD serialNumber = 0;
    DWORD maxComponentLength = 0;
    DWORD flags = 0;

    if (GetVolumeInformationA(
            driveLetter,
            volumeName,
            sizeof(volumeName),
            &serialNumber,
            &maxComponentLength,
            &flags,
            fileSystemName,
            sizeof(fileSystemName)))
    {
        if (strstr(volumeName, "Backup") != NULL ||
            strstr(volumeName, "BACKUP") != NULL)
        {
            return TRUE;
        }
    }

    return FALSE;
}

// Program entry point
int main(void)
{
    char key[KEY_BUFFER_SIZE] = {0};

    if (!read_key(
            KEY_PATH,
            key,
            sizeof(key)))
    {
        printf(
            "Unable to read the A01_3 laboratory key.\n");

        return 1;
    }

    /*
     * Restore the controlled backup volume first.
     *
     * This mirrors the current A01_3 encryptor, which invokes
     * encrypt_directory() on the root of the discovered volume.
     */
    const char *backupDrive =
        EnumBackupVolume();

    if (backupDrive != NULL &&
        PathFileExistsA(backupDrive) &&
        PathIsDirectoryA(backupDrive))
    {
        decrypt_directory(
            backupDrive,
            key);
    }

    /*
     * Restore the primary controlled test directory.
     */
    if (PathFileExistsA(PRIMARY_TARGET) &&
        PathIsDirectoryA(PRIMARY_TARGET))
    {
        decrypt_directory(
            PRIMARY_TARGET,
            key);
    }

    SecureZeroMemory(
        key,
        sizeof(key));

    return 0;
}
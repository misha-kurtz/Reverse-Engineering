// A05_2a_dns_beacon.cpp
// uses getaddrinfo to perform DNS lookups for a series of generated hostnames

/*
builds DNS query (ex. tick-0001.beacon01.lab.test)
performs a DNS lookup with getaddrinfo
sleeps for a fixed interval
repeats for a fixed number of iterations
*/

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "Ws2_32.lib")

std::string build_query_name(const std::string &base_domain, int counter)
{
    std::ostringstream oss;
    oss << "tick-" << std::setw(4) << std::setfill('0') << counter
        << "." << base_domain;
    return oss.str();
}

bool perform_dns_lookup(const std::string &hostname)
{
    addrinfo hints{};
    hints.ai_family = AF_INET; // IPv4 A record only
    hints.ai_socktype = 0;
    hints.ai_protocol = 0;

    addrinfo *result = nullptr;
    int rc = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);

    if (result != nullptr)
    {
        freeaddrinfo(result);
    }

    return rc == 0;
}

int main()
{
    const std::string base_domain = "beacon01.lab.local";
    const int iterations = 10;
    const DWORD sleep_ms = 15000; // 15 seconds

    WSADATA wsaData{};
    int wsa_rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsa_rc != 0)
    {
        std::cerr << "WSAStartup failed: " << wsa_rc << std::endl;
        return 1;
    }

    std::cout << "[*] Starting A05_2a safe DNS signaling simulator" << std::endl;
    std::cout << "[*] Base domain: " << base_domain << std::endl;
    std::cout << "[*] Iterations: " << iterations << std::endl;
    std::cout << "[*] Sleep: " << sleep_ms << " ms" << std::endl;

    for (int i = 1; i <= iterations; ++i)
    {
        std::string query = build_query_name(base_domain, i);
        bool ok = perform_dns_lookup(query);

        std::cout << "[*] Query " << i << ": " << query
                  << " -> " << (ok ? "resolved" : "not resolved") << std::endl;

        if (i < iterations)
        {
            Sleep(sleep_ms);
        }
    }

    WSACleanup();
    std::cout << "[*] Finished." << std::endl;
    return 0;
}
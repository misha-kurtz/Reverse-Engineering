// A05_2b_DnsQueryA_dns_signal.cpp
// Uses DnsQuery_A to perform DNS lookups for a series of generated hostnames

/*
builds a DNS query (ex. tick-0001.beacon01.lab.test
performs a DNS lookup with DnsQuery_A
sleeps for a fixed interval
repeats for a fixed number of iterations
*/

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <windns.h>

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Dnsapi.lib")

std::string build_query_name(const std::string &base_domain, int counter)
{
    std::ostringstream oss;
    oss << "tick-" << std::setw(4) << std::setfill('0') << counter
        << "." << base_domain;
    return oss.str();
}

bool perform_dns_a_query(const std::string &hostname)
{
    PDNS_RECORDA pRecord = nullptr;

    DNS_STATUS status = DnsQuery_A(
        hostname.c_str(),
        DNS_TYPE_A,
        DNS_QUERY_STANDARD,
        nullptr,
        reinterpret_cast<PDNS_RECORD *>(&pRecord),
        nullptr);

    if (pRecord != nullptr)
    {
        DnsRecordListFree(pRecord, DnsFreeRecordList);
    }

    return status == 0;
}

int main()
{
    const std::string base_domain = "beacon02.lab.local";
    const int iterations = 10;
    const DWORD sleep_ms = 15000;

    WSADATA wsaData{};
    int wsa_rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsa_rc != 0)
    {
        std::cerr << "WSAStartup failed: " << wsa_rc << std::endl;
        return 1;
    }

    std::cout << "[*] Starting A05_2b safe DNS signaling simulator\n";
    std::cout << "[*] Base domain: " << base_domain << "\n";

    for (int i = 1; i <= iterations; ++i)
    {
        std::string query = build_query_name(base_domain, i);
        bool ok = perform_dns_a_query(query);

        std::cout << "[*] Query " << i << ": " << query
                  << " -> " << (ok ? "resolved" : "not resolved") << "\n";

        if (i < iterations)
        {
            Sleep(sleep_ms);
        }
    }

    WSACleanup();
    std::cout << "[*] Finished.\n";
    return 0;
}
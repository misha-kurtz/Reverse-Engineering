#include <windows.h>
#include <fstream>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

// Function to append a timestamped log entry to our technique validation file
void LogPersistenceActivity(const std::string &logFilePath)
{
    std::ofstream logFile;
    // Open in append mode so we don't overwrite previous entries
    logFile.open(logFilePath, std::ios_base::app);

    if (logFile.is_open())
    {
        // Get the current system time
        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

        // Strip newline from ctime output
        std::string timeStr = std::ctime(&currentTime);
        if (!timeStr.empty() && timeStr.back() == '\n')
        {
            timeStr.pop_back();
        }

        // Write telemetry validation data
        logFile << "[" << timeStr << "] [A06_3c] Legacy COM API Task execution active. Context: SYSTEM/Admin." << std::endl;
        logFile.close();
    }
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Target validation file path matching your public folder data matrix
    std::string logFilePath = "C:\\Users\\Public\\A06_3c_COM_API_Scheduled_Task_SampleApp_log.txt";

    // Immediate initial callback write to verify execution triggered instantly on logon
    LogPersistenceActivity(logFilePath);

    // Continuous background telemetry loop executing every 5 seconds
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        LogPersistenceActivity(logFilePath);
    }

    return 0;
}
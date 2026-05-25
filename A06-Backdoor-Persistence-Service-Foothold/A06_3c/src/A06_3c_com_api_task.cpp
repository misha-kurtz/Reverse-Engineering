#define _WIN32_DCOM
#include <windows.h>
#include <initguid.h>
#include <taskschd.h> // Modern Task Scheduler 2.0 Header
#include <iostream>
#include <comdef.h>

// Link with modern task scheduler and COM libraries
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

int main()
{
    HRESULT hr = S_OK;

    // 1. Initialize COM Library
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        std::clog << "[-] CoInitializeEx failed: 0x" << std::hex << hr << std::endl;
        return 1;
    }

    // 2. Initialize COM Security
    hr = CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_PKT_PRIVACY, // Recommended encryption level for service interaction
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE,
        NULL);

    if (FAILED(hr))
    {
        std::clog << "[-] CoInitializeSecurity failed: 0x" << std::hex << hr << std::endl;
        CoUninitialize();
        return 1;
    }

    // 3. Create instance of the Modern Task Scheduler Service
    ITaskService *pService = NULL;
    hr = CoCreateInstance(
        CLSID_TaskScheduler,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ITaskService,
        (void **)&pService);

    if (FAILED(hr))
    {
        std::clog << "[-] Failed to create ITaskService instance: 0x" << std::hex << hr << std::endl;
        CoUninitialize();
        return 1;
    }

    // Connect to the local Task Scheduler service
    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr))
    {
        std::clog << "[-] Service connection failed: 0x" << std::hex << hr << std::endl;
        pService->Release();
        CoUninitialize();
        return 1;
    }

    // 4. Open the Root Task Folder ("\")
    ITaskFolder *pRootFolder = NULL;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr))
    {
        std::clog << "[-] Failed to get Root Folder: 0x" << std::hex << hr << std::endl;
        pService->Release();
        CoUninitialize();
        return 1;
    }

    // Safely remove any older conflicting iterations of the task
    pRootFolder->DeleteTask(_bstr_t(L"A06_3c_COM_API_Task"), 0);

    // 5. Create a New Blank Task Definition Object
    ITaskDefinition *pTask = NULL;
    hr = pService->NewTask(0, &pTask);
    if (FAILED(hr))
    {
        std::clog << "[-] Failed to create NewTask definition: 0x" << std::hex << hr << std::endl;
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();
        return 1;
    }

    // [Step A]: Define Registration Info (Metadata)
    IRegistrationInfo *pRegInfo = NULL;
    hr = pTask->get_RegistrationInfo(&pRegInfo);
    if (SUCCEEDED(hr))
    {
        pRegInfo->put_Author(_bstr_t(L"A06_3c_Sample"));
        pRegInfo->put_Description(_bstr_t(L"Modern COM API Native Scheduled Task"));
        pRegInfo->Release();
    }

    // [Step B]: Define Principal (Privilege and Identity settings)
    IPrincipal *pPrincipal = NULL;
    hr = pTask->get_Principal(&pPrincipal);
    if (SUCCEEDED(hr))
    {
        pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);

        // Semantically explicit edits for SYSTEM context assignment
        pPrincipal->put_UserId(_bstr_t(L"SYSTEM"));
        pPrincipal->put_LogonType(TASK_LOGON_SERVICE_ACCOUNT);

        pPrincipal->Release();
    }

    // [Step C]: Define Trigger (Time Event with 1-Minute Repetition)
    ITriggerCollection *pTriggerCollection = NULL;
    hr = pTask->get_Triggers(&pTriggerCollection);
    if (SUCCEEDED(hr))
    {
        ITrigger *pTrigger = NULL;
        hr = pTriggerCollection->Create(TASK_TRIGGER_TIME, &pTrigger);
        if (SUCCEEDED(hr))
        {
            ITimeTrigger *pTimeTrigger = NULL;
            hr = pTrigger->QueryInterface(IID_ITimeTrigger, (void **)&pTimeTrigger);
            if (SUCCEEDED(hr))
            {
                pTimeTrigger->put_Id(_bstr_t(L"TimeTrigger_A06_3c"));

                // Calculate current local time + 1 minute for a clean start boundary
                SYSTEMTIME st;
                GetLocalTime(&st);

                // Convert to a filetime structures to seamlessly roll forward cleanly
                // over boundary crossings (like hour/day shifts)
                FILETIME ft;
                SystemTimeToFileTime(&st, &ft);
                ULARGE_INTEGER uli;
                uli.LowPart = ft.dwLowDateTime;
                uli.HighPart = ft.dwHighDateTime;

                // Add 1 minute (60 seconds * 10,000,000 intervals per second)
                uli.QuadPart += 60ULL * 10000000ULL;

                ft.dwLowDateTime = uli.LowPart;
                ft.dwHighDateTime = uli.HighPart;
                FileTimeToSystemTime(&ft, &st);

                // Format dynamically into the required ISO 8601 string style
                wchar_t startBoundary[32];
                swprintf_s(startBoundary, 32, L"%04d-%02d-%02dT%02d:%02d:%02d",
                           st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

                pTimeTrigger->put_StartBoundary(_bstr_t(startBoundary));

                // Configure the Repetition Pattern
                IRepetitionPattern *pRepetition = NULL;
                hr = pTimeTrigger->get_Repetition(&pRepetition);
                if (SUCCEEDED(hr))
                {
                    pRepetition->put_Interval(_bstr_t(L"PT1M")); // Repeat every 1 minute
                    pRepetition->Release();
                }
                pTimeTrigger->Release();
            }
            pTrigger->Release();
        }
        pTriggerCollection->Release();
    }

    // [Step D]: Define Execution Action (What application to execute)
    IActionCollection *pActionCollection = NULL;
    hr = pTask->get_Actions(&pActionCollection);
    if (SUCCEEDED(hr))
    {
        IAction *pAction = NULL;
        hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
        if (SUCCEEDED(hr))
        {
            IExecAction *pExecAction = NULL;
            hr = pAction->QueryInterface(IID_IExecAction, (void **)&pExecAction);
            if (SUCCEEDED(hr))
            {
                pExecAction->put_Path(_bstr_t(L"C:\\Users\\Public\\A06_3c_SampleApp.exe"));
                pExecAction->Release();
            }
            pAction->Release();
        }
        pActionCollection->Release();
    }

    // Task settings configurations
    ITaskSettings *pSettings = NULL;
    hr = pTask->get_Settings(&pSettings);
    if (SUCCEEDED(hr))
    {
        pSettings->put_Hidden(VARIANT_TRUE); // Hide inside typical views
        pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
        pSettings->put_StopIfGoingOnBatteries(VARIANT_FALSE);
        pSettings->Release();
    }

    // 6. Register Task Definition Natively Into Windows Folder Base
    IRegisteredTask *pRegisteredTask = NULL;
    hr = pRootFolder->RegisterTaskDefinition(
        _bstr_t(L"A06_3c_COM_API_Task"),
        pTask,
        TASK_CREATE_OR_UPDATE,
        _variant_t(L"SYSTEM"),      // Execution account
        _variant_t(),               // Password (Not needed for SYSTEM)
        TASK_LOGON_SERVICE_ACCOUNT, // Run as service context
        _variant_t(L""),
        &pRegisteredTask);

    if (SUCCEEDED(hr))
    {
        std::cout << "[+] Successfully registered native Task Scheduler 2.0 COM object!" << std::endl;
        std::cout << "[*] System Registration complete: Task is active in Root Folder Library." << std::endl;
        pRegisteredTask->Release();
    }
    else
    {
        std::clog << "[-] Failed to register native Task Definition: 0x" << std::hex << hr << std::endl;
    }

    // Clean up allocated references
    pTask->Release();
    pRootFolder->Release();
    pService->Release();
    CoUninitialize();

    return SUCCEEDED(hr) ? 0 : 1;
}
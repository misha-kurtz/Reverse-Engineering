- ==Add "expected_analysis_artifacts" section  to metadata 
- ==update table to match actual analysis artifacts based on the logs I collect at runtime

### A02 Controlled Injection Behavior Class

|ID|Technique|Dominant APIs / Mechanisms|Static Artifacts (Ghidra)|Dynamic Artifacts (Procmon / Sysmon / ETW)|Semantic Meaning|
|---|---|---|---|---|---|
|**A02_1**|DLL Injection (LoadLibrary)|`OpenProcess`, `VirtualAllocEx`, `WriteProcessMemory`, `CreateRemoteThread`, `LoadLibraryA`|Clear injection chain (alloc → write → thread), embedded DLL path string, `LoadLibraryA` resolution via `GetProcAddress`|Handle opened to target process, memory allocated and written in remote process, remote thread execution, **DLL loaded into target**, **target process creates marker file (`C:\Users\Public\A02_1_Injected_OK.txt`)**|Forces a target process to load an external DLL via the OS loader, resulting in **execution of attacker-controlled code within the target process context**, evidenced by target-originated side effects|
|**A02_2**|Remote Thread Injection (Shellcode)|`CreateToolhelp32Snapshot`, `Process32First/Next`, `OpenProcess`, `VirtualAllocEx`, `WriteProcessMemory`, `VirtualProtectEx`, `CreateRemoteThread`|Large embedded shellcode byte array, absence of DLL path strings, memory protection transition logic (`RW → RX`), direct execution pointer usage|Target process handle opened, remote memory allocated and written, memory protection changed to executable, **remote thread starts at private memory**, **calc.exe spawned from target process**, no DLL load event|Executes raw position-independent code directly inside another process’s memory, bypassing the OS loader and executing from attacker-controlled memory regions|
|**A02_3**|Thread Hijacking (Local Context Manipulation)|`CreateThread` (suspended), `GetThreadContext`, `SetThreadContext`, `ResumeThread`, `VirtualAlloc`, `VirtualProtect`, `RtlCopyMemory`|Embedded shellcode, explicit `CONTEXT` structure usage, RIP/EIP modification, absence of `CreateRemoteThread` and cross-process APIs|Suspended thread created locally, memory allocated and made executable, **thread context modified (RIP redirected)**, thread resumed, **calc.exe spawned**, no remote process interaction|Redirects execution of an existing thread by modifying its CPU context, causing execution of attacker-controlled code without creating a new thread or using cross-process injection|
|**A02_4**|APC Injection (Queued LoadLibrary)|`OpenProcess`, `VirtualAllocEx`, `WriteProcessMemory`, `CreateToolhelp32Snapshot`, `Thread32First/Next`, `OpenThread`, `QueueUserAPC`, `GetProcAddress` (`LoadLibraryA`)|Presence of `QueueUserAPC`, thread enumeration logic, DLL path string, indirect execution trigger, absence of `CreateRemoteThread`|Target process handle opened, memory written in remote process, **APC queued to target thread**, execution occurs when thread enters alertable state (`SleepEx`), **DLL loaded into target**, **marker file created (`C:\Users\Public\A02_4_APC_Injection_OK.txt`)**|Schedules execution of injected code via asynchronous procedure calls, causing a target thread to execute attacker-controlled code only when it becomes alertable|
|**A02_5**|Process Hollowing (RunPE)|`CreateProcess` (suspended), `VirtualAllocEx`, `WriteProcessMemory`, `ReadProcessMemory`, `GetThreadContext`, `SetThreadContext`, `ResumeThread`|PE parsing logic (`IMAGE_NT_HEADERS`), section copying loops, image base manipulation, suspended process creation, absence of `LoadLibrary`/DLL artifacts|Legitimate process created in suspended state, memory of target rewritten with new PE image, thread context redirected to new entry point, **process resumes executing replacement binary**, mismatch between on-disk image and runtime behavior|Replaces the memory image of a legitimate process with a different executable, allowing attacker-controlled code to run under the identity of a benign process|
### Control Samples

1. Simple DLL Injection: https://github.com/misha-kurtz/Reverse-Engineering/tree/main/A02-Process-Inj-Hollowing-Code-Inj/A02_1
2. Remote Thread Injection (shellcode or function pointer): https://github.com/misha-kurtz/Reverse-Engineering/tree/main/A02-Process-Inj-Hollowing-Code-Inj/A02_2
3. Thread Hijacking: https://github.com/misha-kurtz/Reverse-Engineering/tree/main/A02-Process-Inj-Hollowing-Code-Inj/A02_3
4. APC Injection: https://github.com/misha-kurtz/Reverse-Engineering/tree/main/A02-Process-Inj-Hollowing-Code-Inj/A02_4
5. Process Hollowing: https://github.com/misha-kurtz/Reverse-Engineering/tree/main/A02-Process-Inj-Hollowing-Code-Inj/A02_5

### 1. Simple DLL Injection Metadata

``` json
{
  "sample_id": "A02_1",
  "sample_name": "dll_injection_loadlibrary",
  "dataset": "A",
  "dataset_type": "controlled",
  "behavior_class": "A02",
  "behavior_class_name": "Controlled Injection Behavior Class",

  "source_repository": "https://github.com/misha-kurtz/Reverse-Engineering/tree/main/A02-Process-Inj-Hollowing-Code-Inj/A02_1",
  "language": "C++",
  "platform": "Windows",
  "build_architecture": "x64",

  "primary_behavior": "process_injection",
  "technique": "dll_injection",
  "subtechnique": "loadlibrary_remote_thread",
  "execution_model": "loader_based",

  "description": "Injects a benign DLL into a remote process by writing the DLL path into remote memory and starting a remote thread at LoadLibraryA.",

  "components": {
    "injector": {
      "filename": "A02_1_dll_injector.exe",
      "role": "performs cross-process memory write and remote thread creation",
      "source_origin": "custom controlled implementation"
    },
    "target": {
      "filename": "A02_1_target_process.exe",
      "role": "benign lab target process with fixed window title",
      "window_title": "A02_1_target_process"
    },
    "payload": {
      "filename": "A02_1_marker.dll",
      "role": "benign DLL loaded into target process via LoadLibraryA",
      "payload_type": "benign_dll"
    }
  },

  "key_apis": {  
	"injector": [  
		"FindWindowA",  
		"GetWindowThreadProcessId",  
		"OpenProcess",  
		"VirtualAllocEx",  
		"WriteProcessMemory",  
		"GetModuleHandleA",  
		"GetProcAddress",  
		"CreateRemoteThread",  
		"LoadLibraryA",  
		"WaitForSingleObject",  
		"GetExitCodeThread",  
		"VirtualFreeEx"  
	],  
	"payload_dll": [  
		"DllMain",  
		"DisableThreadLibraryCalls",  
		"CreateThread",  
		"GetCurrentProcessId",  
		"GetModuleFileNameA",  
		"GetLocalTime",  
		"CreateFileA",  
		"WriteFile",  
		"CloseHandle",  
		"OutputDebugStringA"  
	],  
	"target": [  
		"RegisterClassA",  
		"CreateWindowExA",  
		"ShowWindow",  
		"UpdateWindow",  
		"GetMessageA",  
		"TranslateMessage",  
		"DispatchMessageA",  
		"DefWindowProcA"  
	]  
},

  "payload_behavior": {  
	"description": "When loaded into the target process, the DLL creates a marker file indicating successful execution in the target process context.",  
	"expected_actions": [  
	"create marker file",  
	"write timestamp, PID, and process path",  
	"emit debug string"  
	],  
	"expected_output_file": "C:\\Users\\Public\\A02_1_Injected_OK.txt",  
	"marker_string": "THESIS_A02_1_DLL_LOADED"  
},

  "code_placement": "DLL path string written into remote process memory",
  "execution_trigger": "Immediate execution via CreateRemoteThread",
  "execution_target": "remote process identified by window title",

  "expected_static_signals": {  
	"injector": [  
		"FindWindowA usage with target window title string",  
		"OpenProcess with PROCESS_ALL_ACCESS",  
		"VirtualAllocEx for remote DLL path storage",  
		"WriteProcessMemory writing DLL path into target process",  
		"GetProcAddress resolving LoadLibraryA from Kernel32.dll",  
		"CreateRemoteThread using LoadLibraryA as the thread start routine",  
		"hardcoded absolute DLL path string"  
	],  
	"payload_dll": [  
		"DllMain with DLL_PROCESS_ATTACH handling",  
		"DisableThreadLibraryCalls usage",  
		"CreateThread call from DLL entry point",  
		"worker thread performs marker file creation",  
		"CreateFileA import",  
		"WriteFile import",  
		"CloseHandle import",  
		"OutputDebugStringA import",  
		"marker file path string",  
		"marker text string"  
	],  
	"target": [  
		"fixed window class string",  
		"fixed window title string",  
		"standard Win32 message loop",  
		"minimal benign GUI process behavior"  
	]  
},

  "expected_dynamic_signals": {  
	"injector": [  
		"target window lookup",  
		"process ID resolution from target window handle",  
		"handle open to remote process",  
		"remote memory allocation",  
		"DLL path written into target process memory",  
		"remote thread creation",  
		"remote thread exit code corresponding to LoadLibraryA result"  
	],  
	"target": [  
		"A02_1_marker.dll loaded into A02_1_target_process.exe",  
		"worker thread created inside target process by payload DLL",  
		"marker file created from target process context",  
		"optional debug output generated by payload DLL"  
	]  
},

  "semantic_meaning": "Loads an external DLL into another process using the operating system loader, causing the target process to execute benign injected code under its own process identity.",

  "control_notes": {
    "benign_intent": true,
    "network_required": false,
    "persistence": false,
    "credential_access": false,
    "destructive_actions": false,
    "payload_effect": "creates marker file as benign execution marker"
  },

  "success_conditions": [  
	"injector locates A02_1_target_process.exe by window title",  
	"remote thread successfully calls LoadLibraryA",  
	"A02_1_marker.dll is loaded into A02_1_target_process.exe",  
	"C:\\Users\\Public\\A02_1_Injected_OK.txt is created",  
	"marker file contains timestamp, PID, process path, and THESIS_A02_1_DLL_LOADED marker string"  
	]
}
```

### 2. Remote Thread Injection (Controlled Payload) Metadata

``` json
{
  "sample_id": "A02_2",
  "sample_name": "remote_thread_shellcode_injection",
  "dataset": "A",
  "dataset_type": "controlled",
  "behavior_class": "A02",
  "behavior_class_name": "Controlled Injection Behavior Class",

  "source_repository": "https://github.com/misha-kurtz/Reverse-Engineering/tree/main/A02-Process-Inj-Hollowing-Code-Inj/A02_2",
  "language": "C++",
  "platform": "Windows",
  "build_architecture": "x64",

  "primary_behavior": "process_injection",
  "technique": "remote_thread_injection",
  "subtechnique": "shellcode_remote_thread",
  "execution_model": "direct_code_execution",

  "description": "Injects embedded x64 shellcode into a remote process by allocating memory, writing the payload, changing memory permissions to executable, and starting a remote thread at the injected buffer address.",

  "components": {
    "injector": {
      "filename": "A02_2_remote_thread_inject.exe",
      "role": "performs process enumeration, cross-process memory allocation, shellcode write, memory protection change, and remote thread creation",
      "source_origin": "custom controlled implementation"
    },
    "payload": {
      "filename": "embedded_shellcode",
      "role": "embedded x64 shellcode executed inside the remote process",
      "payload_type": "msfvenom_windows_x64_exec_calc",
      "payload_generation": "msfvenom -p windows/x64/exec CMD=calc.exe EXITFUNC=thread -f c"
    }
  },

  "key_apis": {
    "injector": [
      "CreateToolhelp32Snapshot",
      "Process32First",
      "Process32Next",
      "OpenProcess",
      "VirtualAllocEx",
      "WriteProcessMemory",
      "VirtualProtectEx",
      "CreateRemoteThread",
      "GetThreadId",
      "VirtualFreeEx",
      "CloseHandle",
      "GetLastError"
    ],
    "payload": [
      "position-independent shellcode",
      "PEB walking",
      "API hashing or dynamic API resolution",
      "WinExec or CreateProcess-style execution behavior",
      "ExitThread-compatible termination"
    ]
  },

  "payload_behavior": {
    "description": "The embedded x64 shellcode executes calc.exe from inside the target process context.",
    "expected_actions": [
      "execute injected shellcode",
      "resolve required Windows APIs dynamically",
      "launch calc.exe",
      "terminate shellcode thread"
    ],
    "payload_command": "calc.exe",
    "payload_exit_function": "thread"
  },

  "code_placement": "Raw shellcode bytes written directly into remote process memory",
  "execution_trigger": "Immediate execution via CreateRemoteThread",
  "execution_target": "remote process identified by process image name",

  "expected_static_signals": {
    "injector": [
      "large embedded unsigned char payload buffer",
      "calc.exe string embedded near shellcode tail",
      "CreateToolhelp32Snapshot process enumeration",
      "Process32First and Process32Next process walking",
      "OpenProcess with PROCESS_ALL_ACCESS",
      "VirtualAllocEx allocating remote memory",
      "WriteProcessMemory writing raw payload bytes",
      "VirtualProtectEx changing memory protection to PAGE_EXECUTE_READ",
      "CreateRemoteThread using remote buffer address as thread start routine",
      "absence of LoadLibraryA-based DLL path injection"
    ],
    "payload": [
      "position-independent x64 shellcode bytes",
      "PEB traversal patterns",
      "hashed API resolution constants",
      "embedded calc.exe command string",
      "thread-based exit behavior"
    ]
  },

  "expected_dynamic_signals": {
    "injector": [
      "process snapshot enumeration",
      "target process discovered by executable name",
      "handle open to remote process",
      "remote PAGE_READWRITE memory allocation",
      "WriteProcessMemory into target process",
      "remote memory protection changed to PAGE_EXECUTE_READ",
      "remote thread creation with start address inside private allocated memory"
    ],
    "target": [
      "remote thread begins execution from injected private memory region",
      "no payload DLL is loaded",
      "calc.exe is spawned from target process context",
      "injected thread exits after payload execution"
    ]
  },

  "semantic_meaning": "Executes raw position-independent code inside another process by writing shellcode into remote memory and starting a remote thread at that memory address, without using the Windows loader or loading an external DLL.",

  "control_notes": {
    "benign_intent": true,
    "network_required": false,
    "persistence": false,
    "credential_access": false,
    "destructive_actions": false,
    "payload_effect": "launches calc.exe as benign execution marker"
  },

  "success_conditions": [
    "injector finds target process by image name",
    "remote memory allocation succeeds",
    "embedded shellcode is written into target process memory",
    "remote memory protection is changed to PAGE_EXECUTE_READ",
    "remote thread is created with start address at injected shellcode buffer",
    "calc.exe is launched"
  ]
}
```

### 3. Thread Hijacking Metadata

``` json
{
  "sample_id": "A02_3",
  "sample_name": "local_thread_hijacking_shellcode",
  "dataset": "A",
  "dataset_type": "controlled",
  "behavior_class": "A02",
  "behavior_class_name": "Controlled Injection Behavior Class",

  "source_repository": "https://github.com/misha-kurtz/Reverse-Engineering/tree/main/A02-Process-Inj-Hollowing-Code-Inj/A02_3",
  "language": "C",
  "platform": "Windows",
  "build_architecture": "x64",

  "primary_behavior": "code_injection",
  "technique": "thread_hijacking",
  "subtechnique": "local_suspended_thread_context_hijack",
  "execution_model": "direct_code_execution",

  "description": "Creates a local suspended thread, allocates memory for embedded x64 shellcode, copies the shellcode into the local process, changes the buffer protection to executable, redirects the suspended thread RIP to the shellcode buffer, and resumes the thread to execute calc.exe.",

  "components": {
    "injector": {
      "filename": "A02_3_thread_hijack.exe",
      "role": "creates a suspended local thread, modifies its execution context, and redirects execution to embedded shellcode",
      "source_origin": "custom controlled implementation"
    },
    "payload": {
      "filename": "embedded_shellcode",
      "role": "embedded x64 shellcode executed by hijacked local thread",
      "payload_type": "custom_x64_winexec_calc_shellcode",
      "payload_generation": "nasm -f win64 calc.asm -o calc.obj; objcopy -O binary calc.obj calc.bin; xxd -i calc.bin"
    }
  },

  "key_apis": {
    "injector": [
      "CreateThread",
      "GetThreadId",
      "VirtualAlloc",
      "RtlCopyMemory",
      "VirtualProtect",
      "GetThreadContext",
      "SetThreadContext",
      "ResumeThread",
      "WaitForSingleObject",
      "VirtualFree",
      "CloseHandle",
      "GetLastError"
    ],
    "payload": [
      "PEB walking",
      "kernel32.dll base resolution",
      "export table walking",
      "WinExec dynamic resolution",
      "calc.exe execution",
      "ret-based shellcode return"
    ]
  },

  "payload_behavior": {
    "description": "The embedded custom x64 shellcode walks the PEB to locate kernel32.dll, parses the export table to resolve WinExec, and launches calc.exe.",
    "expected_actions": [
      "execute injected shellcode from hijacked thread",
      "locate kernel32.dll through PEB traversal",
      "resolve WinExec through export table walking",
      "launch calc.exe",
      "return from shellcode"
    ],
    "payload_command": "calc.exe",
    "payload_api_target": "WinExec"
  },

  "code_placement": "Raw shellcode bytes copied into local private process memory",
  "execution_trigger": "ResumeThread after SetThreadContext redirects RIP to shellcode buffer",
  "execution_target": "suspended local thread created inside the current process",

  "expected_static_signals": {
    "injector": [
      "embedded unsigned char shellcode buffer",
      "CreateThread called with CREATE_SUSPENDED",
      "VirtualAlloc allocating local memory as PAGE_READWRITE",
      "RtlCopyMemory copying shellcode into allocated buffer",
      "VirtualProtect changing memory protection to PAGE_EXECUTE_READ",
      "CONTEXT structure initialized with CONTEXT_ALL",
      "GetThreadContext retrieves suspended thread context",
      "SetThreadContext modifies thread RIP",
      "ResumeThread starts execution from modified RIP",
      "absence of OpenProcess, VirtualAllocEx, WriteProcessMemory, and CreateRemoteThread"
    ],
    "payload": [
      "x64 shellcode bytes",
      "PEB traversal using GS segment access",
      "kernel32.dll discovery logic",
      "manual PE export table parsing",
      "WinExec string or little-endian WinExec constant",
      "calc.exe string embedded in shellcode",
      "stack-based argument setup for WinExec"
    ]
  },

  "expected_dynamic_signals": {
    "injector": [
      "local suspended thread creation",
      "local private memory allocation",
      "shellcode copied into local memory",
      "local memory protection changed from PAGE_READWRITE to PAGE_EXECUTE_READ",
      "thread context read using GetThreadContext",
      "thread instruction pointer redirected to shellcode buffer",
      "thread resumed with ResumeThread"
    ],
    "process": [
      "execution begins from private executable memory inside A02_3_thread_hijack.exe",
      "calc.exe is spawned by A02_3_thread_hijack.exe",
      "no remote process handle is required",
      "no DLL payload is loaded",
      "no cross-process WriteProcessMemory activity occurs"
    ]
  },

  "expected_analysis_artifacts": {
    "procmon": [
      "A02_3_thread_hijack.exe creates calc.exe",
      "no marker DLL image load associated with injection",
      "no remote target process interaction required",
      "no file artifact required for success"
    ],
    "sysmon": [
      "process creation event for calc.exe if Event ID 1 is configured",
      "parent process for calc.exe is A02_3_thread_hijack.exe",
      "no process access event to a separate target process is expected",
      "no image load event for an injected DLL payload is expected"
    ],
    "memory": [
      "private local memory region containing custom shellcode",
      "memory region transitions from PAGE_READWRITE to PAGE_EXECUTE_READ",
      "hijacked thread RIP points to private executable memory",
      "calc.exe and WinExec artifacts recoverable from shellcode bytes"
    ]
  },

  "semantic_meaning": "Redirects execution of a suspended local thread by modifying its CPU context so that the thread begins executing shellcode from private executable memory instead of its original thread start routine.",

  "control_notes": {
    "benign_intent": true,
    "network_required": false,
    "persistence": false,
    "credential_access": false,
    "destructive_actions": false,
    "payload_effect": "launches calc.exe as benign execution marker",  
	"cross_process_injection": false
  },

  "success_conditions": [
    "suspended local thread is created successfully",
    "local shellcode buffer is allocated and populated",
    "shellcode buffer protection is changed to PAGE_EXECUTE_READ",
    "GetThreadContext retrieves the suspended thread context",
    "SetThreadContext redirects RIP to the shellcode buffer",
    "ResumeThread starts the hijacked thread",
    "calc.exe is launched"
  ]
}
```

### 4. APC Injection Metadata

``` json
{
  "sample_id": "A02_4",
  "sample_name": "apc_dll_injection_loadlibrary",
  "dataset": "A",
  "dataset_type": "controlled",
  "behavior_class": "A02",
  "behavior_class_name": "Controlled Injection Behavior Class",

  "source_repository": "https://github.com/misha-kurtz/Reverse-Engineering/tree/main/A02-Process-Inj-Hollowing-Code-Inj/A02_4",
  "language": "C# and C++",
  "platform": "Windows",
  "build_architecture": "x64",

  "primary_behavior": "process_injection",
  "technique": "apc_injection",
  "subtechnique": "queueuserapc_loadlibrary",
  "execution_model": "loader_based_apc_delivery",

  "description": "Injects a benign DLL into a remote process by writing the DLL path into remote memory and queuing a user-mode APC that calls LoadLibraryA on one or more target threads. The target process includes an alertable thread that repeatedly enters SleepEx with alertable execution enabled.",

  "components": {
    "injector": {
      "filename": "A02_4_APC_injector.exe",
      "role": "finds the target process, writes the DLL path into remote memory, enumerates target threads, and queues a LoadLibraryA APC",
      "source_origin": "custom controlled implementation",
      "language": "C#"
    },
    "target": {
      "filename": "A02_4_target_process.exe",
      "role": "benign lab target process with a fixed window title and dedicated alertable APC thread",
      "window_title": "A02_4_apc_target_process",
      "language": "C++"
    },
    "payload": {
      "filename": "A02_4_marker.dll",
      "role": "benign DLL loaded into target process through APC-triggered LoadLibraryA execution",
      "payload_type": "benign_dll",
      "language": "C++"
    }
  },

  "key_apis": {
    "injector": [
      "System.Diagnostics.Process.GetProcessesByName",
      "OpenProcess",
      "VirtualAllocEx",
      "WriteProcessMemory",
      "CreateToolhelp32Snapshot",
      "Thread32First",
      "Thread32Next",
      "OpenThread",
      "GetModuleHandle",
      "GetProcAddress",
      "QueueUserAPC",
      "CloseHandle"
    ],
    "target": [
      "CreateThread",
      "SleepEx",
      "OutputDebugStringA",
      "RegisterClassA",
      "CreateWindowExA",
      "ShowWindow",
      "UpdateWindow",
      "GetMessageA",
      "TranslateMessage",
      "DispatchMessageA",
      "DefWindowProcA"
    ],
    "payload_dll": [
      "DllMain",
      "DisableThreadLibraryCalls",
      "CreateThread",
      "GetCurrentProcessId",
      "GetModuleFileNameA",
      "GetLocalTime",
      "CreateFileA",
      "WriteFile",
      "CloseHandle",
      "OutputDebugStringA"
    ]
  },

  "payload_behavior": {
    "description": "When loaded into the target process by an APC-delivered LoadLibraryA call, the DLL creates a marker file indicating successful APC-based DLL injection.",
    "expected_actions": [
      "load marker DLL into target process",
      "create worker thread from DLL_PROCESS_ATTACH",
      "create marker file",
      "write timestamp, PID, and process path",
      "emit debug string"
    ],
    "expected_output_file": "C:\\Users\\Public\\A02_4_APC_Injection_OK.txt",
    "marker_string": "THESIS_A02_4_DLL_LOADED_VIA_APC_INJECTION"
  },

  "code_placement": "DLL path string written into remote process memory",
  "execution_trigger": "Queued user-mode APC calling LoadLibraryA when a target thread enters an alertable state",
  "execution_target": "remote process identified by process image name, with APC delivery to target-owned threads",

  "expected_static_signals": {
    "injector": [
      "C# P/Invoke declarations for kernel32.dll APIs",
      "Process.GetProcessesByName target process lookup",
      "OpenProcess with PROCESS_VM_WRITE and PROCESS_VM_OPERATION access",
      "VirtualAllocEx allocating remote memory for DLL path",
      "WriteProcessMemory writing DLL path into target process",
      "CreateToolhelp32Snapshot with TH32CS_SNAPTHREAD",
      "Thread32First and Thread32Next thread enumeration",
      "OpenThread with SET_CONTEXT access",
      "GetProcAddress resolving LoadLibraryA from kernel32",
      "QueueUserAPC queuing LoadLibraryA to target threads",
      "absence of CreateRemoteThread"
    ],
    "target": [
      "dedicated alertable thread created with CreateThread",
      "SleepEx called with alertable parameter set to TRUE",
      "fixed window class string",
      "fixed window title string",
      "standard Win32 message loop",
      "debug string indicating alertable APC thread startup"
    ],
    "payload_dll": [
      "DllMain with DLL_PROCESS_ATTACH handling",
      "DisableThreadLibraryCalls usage",
      "CreateThread call from DLL entry point",
      "worker thread performs marker file creation",
      "CreateFileA import",
      "WriteFile import",
      "CloseHandle import",
      "OutputDebugStringA import",
      "marker file path string",
      "marker text string"
    ]
  },

  "expected_dynamic_signals": {
    "injector": [
      "target process discovered by process image name",
      "handle open to remote process",
      "remote memory allocation for DLL path",
      "DLL path written into target process memory",
      "system thread snapshot enumeration",
      "target-owned threads opened with SET_CONTEXT access",
      "APC queued to target thread using LoadLibraryA as APC routine",
      "no remote thread is directly created by the injector"
    ],
    "target": [
      "alertable thread repeatedly enters SleepEx alertable state",
      "queued APC executes when alertable thread is scheduled",
      "A02_4_marker.dll loaded into A02_4_target_process.exe",
      "worker thread created inside target process by payload DLL",
      "marker file created from target process context",
      "optional debug output generated by target and payload DLL"
    ]
  },

  "semantic_meaning": "Uses APC delivery to cause an existing target thread to call LoadLibraryA with a remotely written DLL path, loading a benign payload DLL into the target process without directly creating a remote thread.",

  "control_notes": {
    "benign_intent": true,
    "network_required": false,
    "persistence": false,
    "credential_access": false,
    "destructive_actions": false,
    "payload_effect": "creates marker file as benign execution marker",
    "requires_alertable_thread": true
  },

  "success_conditions": [
    "injector finds A02_4_target_process.exe by process name",
    "remote memory allocation for DLL path succeeds",
    "A02_4_marker.dll path is written into target process memory",
    "target process threads are enumerated",
    "APC is queued with LoadLibraryA as the APC routine",
    "target alertable thread enters SleepEx alertable state",
    "A02_4_marker.dll is loaded into A02_4_target_process.exe",
    "C:\\Users\\Public\\A02_4_APC_Injection_OK.txt is created",
    "marker file contains timestamp, PID, process path, and THESIS_A02_4_DLL_LOADED_VIA_APC_INJECTION marker string"
  ]
}
```

### 5. Process Hollowing Metadata

``` json
{
  "sample_id": "A02_5",
  "sample_name": "process_hollowing_runpe",
  "dataset": "A",
  "dataset_type": "controlled",
  "behavior_class": "A02",
  "behavior_class_name": "Controlled Injection Behavior Class",

  "source_repository": "https://github.com/misha-kurtz/Reverse-Engineering/tree/main/A02-Process-Inj-Hollowing-Code-Inj/A02_5",
  "language": "C++",
  "platform": "Windows",
  "build_architecture": "x64",

  "primary_behavior": "process_injection",
  "technique": "process_hollowing",
  "subtechnique": "runpe_suspended_process_image_replacement",
  "execution_model": "image_replacement",

  "description": "Creates a target process in a suspended state, reads a replacement PE image from disk, maps the replacement image into the target process, updates the target process PEB and thread context, and resumes the original thread so the replacement executable runs under the target process identity.",

  "components": {
    "hollower": {
      "filename": "A02_5_process_hollowing.exe",
      "role": "creates a suspended target process and replaces its in-memory image with a supplied PE executable",
      "source_origin": "custom controlled implementation"
    },
    "payload": {
      "filename": "A02_5_loaded_exe.exe",
      "role": "replacement PE image mapped into the suspended target process",
      "payload_type": "benign_executable"
    },
    "target": {
      "filename": "user-supplied target process path",
      "role": "legitimate process image initially created in suspended state and then hollowed"
    }
  },

  "key_apis": {
    "hollower": [
      "CreateFileA",
      "GetFileSize",
      "ReadFile",
      "HeapAlloc",
      "HeapFree",
      "CreateProcessA",
      "IsWow64Process",
      "GetThreadContext",
      "Wow64GetThreadContext",
      "ReadProcessMemory",
      "VirtualAllocEx",
      "WriteProcessMemory",
      "SetThreadContext",
      "Wow64SetThreadContext",
      "ResumeThread",
      "TerminateProcess",
      "CloseHandle"
    ],
    "payload": [
      "replacement PE entry point",
      "PE headers",
      "section table",
      "relocation table if present"
    ]
  },

  "payload_behavior": {
    "description": "The supplied PE payload is manually mapped into the suspended target process and executed from the replacement image entry point.",
    "expected_actions": [
      "read replacement PE from disk",
      "validate PE headers",
      "map PE headers into target process memory",
      "map PE sections into target process memory",
      "apply base relocations if needed",
      "update target PEB image base",
      "redirect suspended thread context to replacement entry point",
      "resume target thread"
    ],
    "payload_file": "A02_5_loaded_exe.exe"
  },

  "code_placement": "Replacement PE headers and sections written into the suspended target process address space",
  "execution_trigger": "ResumeThread after SetThreadContext redirects the suspended primary thread to the replacement PE entry point",
  "execution_target": "suspended target process created with CreateProcessA and CREATE_SUSPENDED",

  "expected_static_signals": {
    "hollower": [
      "CreateProcessA called with CREATE_SUSPENDED",
      "PE file parsing using IMAGE_DOS_HEADER and IMAGE_NT_HEADERS",
      "architecture checks using IsWow64Process and PE optional header magic",
      "target PEB image base discovery using GetThreadContext or Wow64GetThreadContext",
      "ReadProcessMemory reading target image headers and PEB fields",
      "VirtualAllocEx allocating space for replacement image in target process",
      "WriteProcessMemory writing PE headers into target process",
      "WriteProcessMemory writing PE sections into target process",
      "relocation handling using IMAGE_BASE_RELOCATION entries",
      "WriteProcessMemory updating target PEB ImageBaseAddress",
      "SetThreadContext or Wow64SetThreadContext redirecting execution to replacement entry point",
      "ResumeThread starting the hollowed process",
      "absence of LoadLibraryA DLL injection",
      "absence of CreateRemoteThread"
    ],
    "payload": [
      "valid PE executable loaded from disk",
      "PE headers and section table",
      "entry point used as resumed thread destination",
      "relocation table may be present depending on payload build"
    ]
  },

  "expected_dynamic_signals": {
    "hollower": [
      "replacement executable opened and read from disk",
      "target process created in suspended state",
      "target process memory queried or read",
      "remote memory allocation in target process",
      "replacement PE headers written into target process",
      "replacement PE sections written into target process",
      "target PEB image base updated",
      "primary thread context modified",
      "suspended thread resumed"
    ],
    "target": [
      "process image on disk differs from code executing in memory",
      "target process begins execution at replacement PE entry point",
      "payload behavior occurs under the target process identity",
      "no payload DLL image load is required",
      "no new remote thread is required"
    ]
  },
  
  "semantic_meaning": "Replaces the executable image of a suspended process with a different PE image and resumes the original thread so the payload executes while appearing to originate from the target process.",

  "control_notes": {
    "benign_intent": true,
    "network_required": false,
    "persistence": false,
    "credential_access": false,
    "destructive_actions": false,
    "payload_effect": "executes controlled replacement PE as benign execution marker",
    "requires_architecture_match": true,
    "requires_subsystem_match": true
  },

  "success_conditions": [
    "A02_5_process_hollowing.exe reads A02_5_loaded_exe.exe successfully",
    "replacement PE is validated as a valid executable image",
    "target process is created with CREATE_SUSPENDED",
    "source and target architectures are compatible",
    "source and target subsystems are compatible",
    "replacement PE headers are written into the target process",
    "replacement PE sections are written into the target process",
    "relocations are applied if required",
    "target PEB image base is updated",
    "target thread context is redirected to the replacement PE entry point",
    "target process thread is resumed",
    "A02_5_loaded_exe.exe behavior executes under the target process identity"
  ]
}
```
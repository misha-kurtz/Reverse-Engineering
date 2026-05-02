To execute remote_thread_inject.exe:

.\remote_thread_inject.exe <target_process>

.\remote_thread_inject.exe Notepad.exe

1. Injects thread into running Notepad.exe process 
2. Copies payload to thread (msfvenom shellcode to start calc.exe process) 
3. Executes thread which starts calc.exe


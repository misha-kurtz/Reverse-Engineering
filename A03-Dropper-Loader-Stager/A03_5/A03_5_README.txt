A03_5 Fileless Network to Memory Loader

Summary
Downloads a raw payload from a remote HTTP server directly 
into memory using the WinHTTP API, allocates writable memory 
within the current process, stages the downloaded payload 
into the allocated region, changes memory protections from 
writable to executable, and transfers execution directly to 
the downloaded memory buffer via a function pointer call. This 
demonstrates staged network-to-memory loader behavior commonly 
associated with malware downloaders, in-memory loaders, and 
fileless execution frameworks that avoid writing the downloaded 
payload itself to disk prior to execution.

Payload Summary
The downloaded payload is a handcrafted 64-bit Windows assembly 
routine authored in NASM (file_artifact.asm) and compiled as a 
raw flat binary (file_artifact.bin). The payload is designed 
as a position-independent callable memory payload which receives 
a pointer to a parameter structure containing resolved WinAPI 
function pointers supplied by the loader at runtime. After 
execution, the payload retrieves the current process identifier,
the executing process path, and the current local timestamp, and
formats execution metadata into an output buffer, creating
the marker file at C:\User\Public. This provides controlled 
proof that the downloaded payload successfully executed entirely 
from memory without staging the payload itself to disk.

To execute A03_5_network_to_mem_loader:
.\A03-Dropper-Loader-Stager\A03_5\bin\A03_5_network_to_mem_loader.exe

#########################################################################

file_artifact.asm High-Level Payload Flow:
1. Receive PARAMS* structure from the loader
2. Resolve WinAPI functions from supplied function pointers
3. Retrieve process metadata:
4. GetCurrentProcessId
5. GetModuleFileNameA
6. GetLocalTime
7. Format execution metadata using wsprintfA
8. Create marker file using CreateFileA
9. Write execution metadata using WriteFile
10. Close file handle using CloseHandle
11. Emit debug string using OutputDebugStringA

Compile into raw payload in Kali:
# Compile directly to flat binary payload
nasm -f bin file_artifact.asm -o file_artifact.bin

#########################################################################

Host payload from Ubuntu Inetsim server

sudo cp file_artifact.bin /var/lib/inetsim/http/fakefiles/file_artifact.bin
sudo chown inetsim:inetsim /var/lib/inetsim/http/fakefiles/file_artifact.bin

Add static fakefile mapping to:
/etc/inetsim/inetsim.conf

http_static_fakefile /file_artifact.bin file_artifact.bin application/octet-stream

Restart inetsim:
sudo systemctl restart inetsim
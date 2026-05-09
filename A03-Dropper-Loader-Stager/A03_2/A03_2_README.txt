A03_2 Network Stager with Disk Drop and Execute 

Summary
Downloads an executable payload from a controlled 
HTTP server, writes it to disk, and launches it as a new 
process. This models a network-based stager where the initial 
program retrieves a second-stage executable at runtime instead 
of carrying the payload internally.

Payload Summary
A03_2_dropped_exe.exe is a benign marker executable used to 
confirm successful network delivery and execution. After 
being downloaded and launched by the stager, it creates a file
artifact at C:\Users\Public. The marker records the timestamp, 
process ID, and process path, proving that the downloaded 
second-stage executable ran successfully.

To execute A03_2_download_and_execute_stager.exe:
.\A03-Dropper-Loader-Stager\A03_2\bin\A03_2_download_and_execute_stager.exe


#########################################################################

Host payload from Ubuntu Inetsim server

sudo cp A03_2_dropped_exe.exe /var/lib/inetsim/http/fakefiles/A03_2_dropped_exe.exe
sudo chown inetsim:inetsim /var/lib/inetsim/http/fakefiles/A03_2_dropped_exe.exe

Add fakefile mapping to:
/etc/inetsim/inetsim.conf

http_fakefile exe A03_2_dropped_exe.exe application/octet-stream

Restart inetsim:
sudo systemctl restart inetsim
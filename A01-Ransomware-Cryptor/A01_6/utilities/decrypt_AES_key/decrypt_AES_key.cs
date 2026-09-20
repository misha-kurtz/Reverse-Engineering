using System;
using System.IO;
using System.Linq;
using System.Net;
using System.Security.Cryptography;
using System.Text;

class Program
{
    static void Main()
    {
        string privateKeyXml = File.ReadAllText("C:\\Users\\misha.kurtz\\Reverse-Engineering\\A01-Ransomware-Cryptor\\A01_4\\utilities\\decrypt_AES_key\\A01_4_private_key.xml");

        string postedValue = File.ReadAllText("C:\\Users\\misha.kurtz\\Reverse-Engineering\\A01-Ransomware-Cryptor\\A01_4\\utilities\\decrypt_AES_key\\posted_aesencrypted.txt");

        postedValue = WebUtility.UrlDecode(postedValue);
        postedValue = postedValue.Trim();
        postedValue = postedValue.Replace("\r", "").Replace("\n", "").Replace(" ", "");

        Console.WriteLine($"Base64 length: {postedValue.Length}");

        byte[] encryptedAesKey = Convert.FromBase64String(postedValue);

        Console.WriteLine($"Ciphertext bytes: {encryptedAesKey.Length}");

        using var rsa = new RSACryptoServiceProvider(2048);
        rsa.FromXmlString(privateKeyXml);

        byte[] decryptedBytes = rsa.Decrypt(encryptedAesKey, false);
        string aesKey = Encoding.UTF8.GetString(decryptedBytes);

        File.WriteAllText("C:\\Users\\misha.kurtz\\Reverse-Engineering\\A01-Ransomware-Cryptor\\A01_4\\utilities\\decrypt_AES_key\\A01_4_Recovered_AES_Key.txt", aesKey);

        Console.WriteLine($"Recovered AES key: {aesKey}");
    }
}
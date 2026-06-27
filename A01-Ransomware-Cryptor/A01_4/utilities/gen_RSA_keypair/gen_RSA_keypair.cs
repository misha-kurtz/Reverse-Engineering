using System;
using System.IO;
using System.Security.Cryptography;

class Program
{
    static void Main()
    {
        using var rsa = new RSACryptoServiceProvider(2048);

        string privateKey = rsa.ToXmlString(true);
        string publicKey = rsa.ToXmlString(false);

        File.WriteAllText("A01_4_private_key.xml", privateKey);
        File.WriteAllText("A01_4_public_key.xml", publicKey);

        Console.WriteLine("RSA key pair generated.");
    }
}
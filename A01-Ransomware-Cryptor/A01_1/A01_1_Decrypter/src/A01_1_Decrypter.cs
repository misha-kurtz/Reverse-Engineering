using System;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Security.Cryptography;
using System.Drawing;

namespace A01_1_Decrypter
{
    public partial class Form1 : Form
    {
        private TextBox textBox1;
        private Label label1;
        private Button button1;
        private Label label2;
        private Label label3;

        private const string KeyPath = @"C:\Users\Public\A01_1_Lab_Encryption_Key.txt";
        private const string TargetPath = @"C:\Users\Public\A01_TestData";

        public Form1()
        {
            InitializeManualComponents();
            this.Load += new EventHandler(Form1_Load);
        }

        private void InitializeManualComponents()
        {
            this.textBox1 = new TextBox();
            this.label1 = new Label();
            this.button1 = new Button();
            this.label2 = new Label();
            this.label3 = new Label();

            this.SuspendLayout();

            textBox1.Location = new Point(83, 15);
            textBox1.Size = new Size(152, 20);

            label1.AutoSize = true;
            label1.Location = new Point(21, 18);
            label1.Text = "Password:";

            button1.Location = new Point(24, 41);
            button1.Size = new Size(211, 31);
            button1.Text = "Decrypt Lab Files";
            button1.Click += new EventHandler(button1_Click);

            label2.AutoSize = true;
            label2.Location = new Point(30, 112);
            label2.Text = "A01_1 lab decrypter\nHidden-Tear derived sample";

            label3.AutoSize = true;
            label3.Font = new Font("Microsoft Sans Serif", 12F);
            label3.ForeColor = Color.ForestGreen;
            label3.Location = new Point(66, 75);
            label3.Text = "Files Decrypted!";
            label3.Visible = false;

            this.ClientSize = new Size(256, 156);
            this.Controls.Add(label3);
            this.Controls.Add(label2);
            this.Controls.Add(button1);
            this.Controls.Add(label1);
            this.Controls.Add(textBox1);

            this.FormBorderStyle = FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.Text = "A01_1 Decrypter";

            this.ResumeLayout(false);
            this.PerformLayout();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            if (File.Exists(KeyPath))
            {
                textBox1.Text = File.ReadAllText(KeyPath).Trim();
            }
        }

        public byte[] AES_Decrypt(byte[] bytesToBeDecrypted, byte[] passwordBytes)
        {
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

                using (var cs = new CryptoStream(ms, AES.CreateDecryptor(), CryptoStreamMode.Write))
                {
                    cs.Write(bytesToBeDecrypted, 0, bytesToBeDecrypted.Length);
                    cs.Close();
                }

                return ms.ToArray();
            }
        }

        public void DecryptFile(string file, string password)
        {
            byte[] bytesToBeDecrypted = File.ReadAllBytes(file);
            byte[] passwordBytes = Encoding.UTF8.GetBytes(password);
            passwordBytes = SHA256.Create().ComputeHash(passwordBytes);

            byte[] bytesDecrypted = AES_Decrypt(bytesToBeDecrypted, passwordBytes);

            File.WriteAllBytes(file, bytesDecrypted);

            string extension = Path.GetExtension(file);
            string result = file.Substring(0, file.Length - extension.Length);

            File.Move(file, result);
        }

        public void DecryptDirectory(string location, string password)
        {
            string[] files = Directory.GetFiles(location);
            string[] childDirectories = Directory.GetDirectories(location);

            for (int i = 0; i < files.Length; i++)
            {
                string extension = Path.GetExtension(files[i]);

                if (extension.Equals(".locked", StringComparison.OrdinalIgnoreCase))
                {
                    try
                    {
                        DecryptFile(files[i], password);
                    }
                    catch
                    {
                        // Ignore individual file errors and continue.
                    }
                }
            }

            for (int i = 0; i < childDirectories.Length; i++)
            {
                try
                {
                    DecryptDirectory(childDirectories[i], password);
                }
                catch
                {
                    // Ignore inaccessible directories and continue.
                }
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            string password = textBox1.Text.Trim();

            if (Directory.Exists(TargetPath) && !string.IsNullOrWhiteSpace(password))
            {
                DecryptDirectory(TargetPath, password);
                label3.Visible = true;
            }
        }
    }
}
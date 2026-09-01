using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace WebSwingControls.SMPCToolCompanion
{
    internal static class Program
    {
        public const string Version = "1.1.0-rc2";

        [STAThread]
        private static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new CompanionForm());
        }

        public static InstallerPolicy CreateProductionPolicy()
        {
            InstallerPolicy policy = new InstallerPolicy();
            policy.ExpectedGameExecutableSha256 = "E297D4D94F1FFE4FEBF289745E79E7B6FA233A788E7A00F480FC77C55DB81AD1";
            policy.ExpectedProxySha256 = "FFB6AD902798D7DF87F6D3FDE3B5A4BAE1E9212A40971B346452A5064768E111";
            policy.ExpectedControlsSha256 = "10F25A79F541731BAF898F28316B4FBE444E96E07C6B47F46E49C55F9AB691FC";
            policy.ReplaceableProxySha256.Add("2218355540C903039D84389237EFEF2CC2C9A1611EBC59C45D51D4767E79C7BF");
            policy.ReplaceableControlsSha256.Add("61C2537DD141D803B177B2988119A284CCC8B30CCAA812947528F3E0B014A223");
            policy.ReplaceableControlsSha256.Add("1E0AC0AE9FA82B59F06B5CBCD2A915FECA8FAEEBD924A1306A5471810CE0EBC8");
            policy.IsProtectedProcessRunning = delegate
            {
                return Process.GetProcessesByName("Spider-Man").Length > 0
                    || Process.GetProcessesByName("crs-video").Length > 0
                    || Process.GetProcessesByName("SMPCTool").Length > 0;
            };
            return policy;
        }
    }

    internal sealed class CompanionForm : Form
    {
        private readonly InstallerCore installer;
        private readonly InstallerPolicy policy;
        private readonly string toolDirectory;
        private readonly string payloadDirectory;
        private readonly TextBox output;
        private readonly Button refreshButton;
        private readonly Button installButton;
        private InstallPlan currentPlan;

        public CompanionForm()
        {
            installer = new InstallerCore();
            policy = Program.CreateProductionPolicy();
            string companionDirectory = Path.GetDirectoryName(Application.ExecutablePath);
            toolDirectory = InstallerCore.LocateToolDirectory(companionDirectory);
            payloadDirectory = Path.Combine(companionDirectory, "payload");

            Text = "Web Swing Controls - SMPCTool Companion " + Program.Version;
            StartPosition = FormStartPosition.CenterScreen;
            MinimumSize = new Size(680, 500);
            Size = new Size(760, 580);
            BackColor = Color.FromArgb(35, 35, 35);
            ForeColor = Color.Gainsboro;
            Font = new Font("Segoe UI", 9F);

            Label heading = new Label();
            heading.AutoSize = false;
            heading.Location = new Point(18, 16);
            heading.Size = new Size(700, 50);
            heading.Font = new Font("Segoe UI Semibold", 15F);
            heading.ForeColor = Color.White;
            heading.Text = "Web Swing Controls\r\nUnofficial companion for jedijosh920's SMPCTool v1.1.1";

            Label explanation = new Label();
            explanation.AutoSize = false;
            explanation.Location = new Point(20, 76);
            explanation.Size = new Size(700, 45);
            explanation.Text =
                "Place this companion folder beside the official SMPCTool.exe. "
                + "It reads SMPCTool's assetArchiveDir.txt but never modifies or redistributes SMPCTool.";

            output = new TextBox();
            output.Location = new Point(20, 128);
            output.Size = new Size(704, 350);
            output.Anchor = AnchorStyles.Top | AnchorStyles.Bottom | AnchorStyles.Left | AnchorStyles.Right;
            output.Multiline = true;
            output.ReadOnly = true;
            output.ScrollBars = ScrollBars.Both;
            output.WordWrap = false;
            output.BackColor = Color.FromArgb(20, 20, 20);
            output.ForeColor = Color.Gainsboro;
            output.Font = new Font("Consolas", 9F);

            refreshButton = new Button();
            refreshButton.Text = "Check Again";
            refreshButton.Location = new Point(20, 494);
            refreshButton.Size = new Size(110, 32);
            refreshButton.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
            refreshButton.Click += delegate { RefreshPlan(); };

            installButton = new Button();
            installButton.Text = "Install";
            installButton.Location = new Point(140, 494);
            installButton.Size = new Size(110, 32);
            installButton.Anchor = AnchorStyles.Bottom | AnchorStyles.Left;
            installButton.Enabled = false;
            installButton.Click += InstallButtonClick;

            Button closeButton = new Button();
            closeButton.Text = "Close";
            closeButton.Location = new Point(614, 494);
            closeButton.Size = new Size(110, 32);
            closeButton.Anchor = AnchorStyles.Bottom | AnchorStyles.Right;
            closeButton.Click += delegate { Close(); };

            Controls.Add(heading);
            Controls.Add(explanation);
            Controls.Add(output);
            Controls.Add(refreshButton);
            Controls.Add(installButton);
            Controls.Add(closeButton);

            Shown += delegate { RefreshPlan(); };
        }

        private void RefreshPlan()
        {
            currentPlan = null;
            installButton.Enabled = false;
            output.Text = "Checking SMPCTool folder and game installation...";
            try
            {
                currentPlan = installer.Inspect(toolDirectory, payloadDirectory, policy);
                output.Text = FormatPlan(currentPlan);
                installButton.Enabled = currentPlan.HasChanges;
            }
            catch (Exception ex)
            {
                output.Text =
                    "CHECK STOPPED\r\n\r\n"
                    + ex.Message
                    + "\r\n\r\nRequired setup:\r\n"
                    + "1. Put this companion folder beside the official SMPCTool.exe.\r\n"
                    + "2. Open SMPCTool and select the game's asset_archive folder.\r\n"
                    + "3. Close SMPCTool and choose Check Again.";
            }
        }

        private void InstallButtonClick(object sender, EventArgs eventArgs)
        {
            if (currentPlan == null || !currentPlan.HasChanges)
            {
                return;
            }

            DialogResult confirmation = MessageBox.Show(
                this,
                "Install Web Swing Controls into this game folder?\r\n\r\n"
                    + currentPlan.GameDirectory
                    + "\r\n\r\nThe game and SMPCTool should be closed. Existing changed files receive verified backups.",
                "Confirm installation",
                MessageBoxButtons.YesNo,
                MessageBoxIcon.Question,
                MessageBoxDefaultButton.Button2
            );
            if (confirmation != DialogResult.Yes)
            {
                return;
            }

            refreshButton.Enabled = false;
            installButton.Enabled = false;
            output.Text = "Installing...";
            try
            {
                InstallResult result = installer.Install(toolDirectory, payloadDirectory, policy);
                output.Text = FormatPlan(result.Plan)
                    + "\r\n\r\nINSTALLATION COMPLETE"
                    + (string.IsNullOrEmpty(result.BackupDirectory)
                        ? string.Empty
                        : "\r\nBackup: " + result.BackupDirectory)
                    + "\r\n\r\nLaunch the game normally through Steam.";
                MessageBox.Show(
                    this,
                    "Web Swing Controls installed successfully.",
                    "Installation complete",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Information
                );
            }
            catch (Exception ex)
            {
                output.Text = "INSTALLATION STOPPED\r\n\r\n" + ex.Message;
                MessageBox.Show(
                    this,
                    ex.Message,
                    "Installation stopped",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error
                );
            }
            finally
            {
                refreshButton.Enabled = true;
            }
        }

        private static string FormatPlan(InstallPlan plan)
        {
            StringBuilder text = new StringBuilder();
            text.AppendLine("CHECK PASSED");
            text.AppendLine();
            text.AppendLine("SMPCTool folder: " + plan.ToolDirectory);
            text.AppendLine("Asset archive:   " + plan.AssetArchiveDirectory);
            text.AppendLine("Game folder:     " + plan.GameDirectory);
            text.AppendLine("SMPCTool SHA-256: " + plan.SMPCToolSha256);
            text.AppendLine("Game EXE SHA-256: " + plan.GameExecutableSha256);
            text.AppendLine();
            text.AppendLine("Planned files:");
            foreach (InstallAction action in plan.Actions)
            {
                text.AppendLine("  " + action.Kind.ToString().ToUpperInvariant().PadRight(9) + action.RelativePath);
            }

            text.AppendLine();
            text.Append(plan.HasChanges ? "Ready to install." : "Already installed; no changes needed.");
            return text.ToString();
        }
    }
}

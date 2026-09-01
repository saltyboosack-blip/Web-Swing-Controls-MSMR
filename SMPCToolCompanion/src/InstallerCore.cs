using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Security.Cryptography;
using System.Text;
using System.Threading;

namespace WebSwingControls.SMPCToolCompanion
{
    public enum InstallActionKind
    {
        Create,
        Replace,
        Unchanged
    }

    public sealed class InstallAction
    {
        public InstallAction(string relativePath, InstallActionKind kind, string existingSha256, string desiredSha256)
        {
            RelativePath = relativePath;
            Kind = kind;
            ExistingSha256 = existingSha256;
            DesiredSha256 = desiredSha256;
        }

        public string RelativePath { get; private set; }
        public InstallActionKind Kind { get; private set; }
        public string ExistingSha256 { get; private set; }
        public string DesiredSha256 { get; private set; }
    }

    public sealed class InstallPlan
    {
        internal InstallPlan(
            string toolDirectory,
            string assetArchiveDirectory,
            string gameDirectory,
            string smpcToolSha256,
            string gameExecutableSha256,
            IList<InstallAction> actions)
        {
            ToolDirectory = toolDirectory;
            AssetArchiveDirectory = assetArchiveDirectory;
            GameDirectory = gameDirectory;
            SMPCToolSha256 = smpcToolSha256;
            GameExecutableSha256 = gameExecutableSha256;
            Actions = new List<InstallAction>(actions).AsReadOnly();
        }

        public string ToolDirectory { get; private set; }
        public string AssetArchiveDirectory { get; private set; }
        public string GameDirectory { get; private set; }
        public string SMPCToolSha256 { get; private set; }
        public string GameExecutableSha256 { get; private set; }
        public IList<InstallAction> Actions { get; private set; }

        public bool HasChanges
        {
            get
            {
                foreach (InstallAction action in Actions)
                {
                    if (action.Kind != InstallActionKind.Unchanged)
                    {
                        return true;
                    }
                }

                return false;
            }
        }
    }

    public sealed class InstallResult
    {
        internal InstallResult(InstallPlan plan, bool changed, string backupDirectory)
        {
            Plan = plan;
            Changed = changed;
            BackupDirectory = backupDirectory;
        }

        public InstallPlan Plan { get; private set; }
        public bool Changed { get; private set; }
        public string BackupDirectory { get; private set; }
    }

    public sealed class InstallerPolicy
    {
        public InstallerPolicy()
        {
            ReplaceableProxySha256 = new List<string>();
            ReplaceableControlsSha256 = new List<string>();
            IsProtectedProcessRunning = DefaultProtectedProcessCheck;
            UtcNow = delegate { return DateTime.UtcNow; };
            NewId = delegate { return Guid.NewGuid().ToString("N"); };
        }

        public string ExpectedGameExecutableSha256 { get; set; }
        public string ExpectedProxySha256 { get; set; }
        public string ExpectedControlsSha256 { get; set; }
        public IList<string> ReplaceableProxySha256 { get; private set; }
        public IList<string> ReplaceableControlsSha256 { get; private set; }
        public Func<bool> IsProtectedProcessRunning { get; set; }
        public Func<DateTime> UtcNow { get; set; }
        public Func<string> NewId { get; set; }

        private static bool DefaultProtectedProcessCheck()
        {
            return Process.GetProcessesByName("Spider-Man").Length > 0
                || Process.GetProcessesByName("crs-video").Length > 0
                || Process.GetProcessesByName("SMPCTool").Length > 0;
        }
    }

    public sealed class InstallerException : Exception
    {
        public InstallerException(string message)
            : base(message)
        {
        }

        public InstallerException(string message, Exception innerException)
            : base(message, innerException)
        {
        }
    }

    public sealed class InstallerCore
    {
        private const string ToolExecutableName = "SMPCTool.exe";
        private const string ToolConfigName = "assetArchiveDir.txt";
        private const string GameExecutableName = "Spider-Man.exe";
        private const string ControlsRelativePath = "scripts\\TrueSwing.dll";
        private const string ScriptsListRelativePath = "scripts.txt";
        private const string ProxyRelativePath = "winmm.dll";
        private const string CommandLineRelativePath = "commandline.txt";
        private const string BackupRootName = "WSC Backups";
        private const int MaximumTextFileBytes = 1024 * 1024;
        private const int LegacyMaximumPathCharacters = 259;
        private const int LegacyMaximumDirectoryPathCharacters = 247;

        private sealed class DesiredFile
        {
            public string RelativePath;
            public string Destination;
            public byte[] Content;
            public InstallAction Action;
        }

        private sealed class PreparedPlan
        {
            public InstallPlan PublicPlan;
            public List<DesiredFile> DesiredFiles;
        }

        private sealed class CommitRecord
        {
            public DesiredFile Desired;
            public bool Existed;
            public string BackupPath;
        }

        public static string LocateToolDirectory(string companionDirectory)
        {
            if (string.IsNullOrWhiteSpace(companionDirectory))
            {
                throw new InstallerException("Companion folder is not set.");
            }

            string resolvedCompanionDirectory = Path.GetFullPath(companionDirectory)
                .TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
            if (File.Exists(Path.Combine(resolvedCompanionDirectory, ToolExecutableName)))
            {
                return resolvedCompanionDirectory;
            }

            DirectoryInfo parent = Directory.GetParent(resolvedCompanionDirectory);
            if (parent != null && File.Exists(Path.Combine(parent.FullName, ToolExecutableName)))
            {
                return parent.FullName;
            }

            return resolvedCompanionDirectory;
        }

        public InstallPlan Inspect(string toolDirectory, string payloadDirectory, InstallerPolicy policy)
        {
            return Prepare(toolDirectory, payloadDirectory, policy).PublicPlan;
        }

        public InstallResult Install(string toolDirectory, string payloadDirectory, InstallerPolicy policy)
        {
            PreparedPlan prepared = Prepare(toolDirectory, payloadDirectory, policy);
            if (!prepared.PublicPlan.HasChanges)
            {
                return new InstallResult(prepared.PublicPlan, false, null);
            }

            if (policy.IsProtectedProcessRunning != null && policy.IsProtectedProcessRunning())
            {
                throw new InstallerException(
                    "Marvel's Spider-Man Remastered, crs-video.exe, or SMPCTool.exe is running. Close it yourself, then retry. This companion never closes processes."
                );
            }

            string mutexName = "Local\\WebSwingControlsSMPCTool_"
                + Sha256Bytes(Encoding.UTF8.GetBytes(prepared.PublicPlan.GameDirectory)).Substring(0, 16);
            using (Mutex mutex = new Mutex(false, mutexName))
            {
                bool acquired = false;
                try
                {
                    try
                    {
                        acquired = mutex.WaitOne(TimeSpan.Zero);
                    }
                    catch (AbandonedMutexException)
                    {
                        acquired = true;
                    }

                    if (!acquired)
                    {
                        throw new InstallerException("Another Web Swing Controls installation is already running for this game folder.");
                    }

                    return Commit(prepared, policy);
                }
                finally
                {
                    if (acquired)
                    {
                        mutex.ReleaseMutex();
                    }
                }
            }
        }

        private static PreparedPlan Prepare(string toolDirectory, string payloadDirectory, InstallerPolicy policy)
        {
            if (policy == null)
            {
                throw new ArgumentNullException("policy");
            }

            ValidatePolicy(policy);
            string resolvedToolDirectory = ResolveDirectory(toolDirectory, "SMPCTool folder");
            string toolExecutable = Path.Combine(resolvedToolDirectory, ToolExecutableName);
            AssertRegularFile(toolExecutable, ToolExecutableName);

            string configPath = Path.Combine(resolvedToolDirectory, ToolConfigName);
            AssertRegularFile(configPath, ToolConfigName);
            if (new FileInfo(configPath).Length > 4096)
            {
                throw new InstallerException(ToolConfigName + " is unexpectedly large.");
            }

            string configuredPath = File.ReadAllText(configPath).Trim().Trim('"');
            if (configuredPath.Length == 0)
            {
                throw new InstallerException(ToolConfigName + " is empty. Select the asset archive folder in SMPCTool first.");
            }

            if (File.Exists(configuredPath) && string.Equals(Path.GetFileName(configuredPath), "toc", StringComparison.OrdinalIgnoreCase))
            {
                configuredPath = Path.GetDirectoryName(Path.GetFullPath(configuredPath));
            }

            string assetArchiveDirectory = ResolveDirectory(configuredPath, "configured asset archive folder");
            DirectoryInfo assetDirectoryInfo = new DirectoryInfo(assetArchiveDirectory);
            if (!string.Equals(assetDirectoryInfo.Name, "asset_archive", StringComparison.OrdinalIgnoreCase))
            {
                throw new InstallerException(
                    ToolConfigName + " does not point to the game's asset_archive folder: " + assetArchiveDirectory
                );
            }

            if (!File.Exists(Path.Combine(assetArchiveDirectory, "toc"))
                && !File.Exists(Path.Combine(assetArchiveDirectory, "toc.BAK")))
            {
                throw new InstallerException("The configured asset_archive folder has neither toc nor toc.BAK.");
            }

            DirectoryInfo parent = assetDirectoryInfo.Parent;
            if (parent == null)
            {
                throw new InstallerException("Could not derive the game folder from asset_archive.");
            }

            string gameDirectory = ResolveDirectory(parent.FullName, "game folder");
            string gameExecutable = Path.Combine(gameDirectory, GameExecutableName);
            AssertRegularFile(gameExecutable, GameExecutableName);
            string gameHash = Sha256File(gameExecutable);
            if (!HashEquals(gameHash, policy.ExpectedGameExecutableSha256))
            {
                throw new InstallerException(
                    "Unsupported Spider-Man.exe build. Expected SHA-256 "
                    + NormalizeHash(policy.ExpectedGameExecutableSha256)
                    + "; found " + gameHash + ". No game files were changed."
                );
            }

            string resolvedPayloadDirectory = ResolveDirectory(payloadDirectory, "companion payload folder");
            string controlsPayload = Path.Combine(resolvedPayloadDirectory, "TrueSwing.dll");
            string proxyPayload = Path.Combine(resolvedPayloadDirectory, "winmm.dll");
            AssertRegularFile(controlsPayload, "payload TrueSwing.dll");
            AssertRegularFile(proxyPayload, "payload winmm.dll");

            byte[] controlsBytes = File.ReadAllBytes(controlsPayload);
            byte[] proxyBytes = File.ReadAllBytes(proxyPayload);
            string controlsHash = Sha256Bytes(controlsBytes);
            string proxyHash = Sha256Bytes(proxyBytes);
            if (!HashEquals(controlsHash, policy.ExpectedControlsSha256))
            {
                throw new InstallerException(
                    "Controls payload failed verification. Expected "
                    + NormalizeHash(policy.ExpectedControlsSha256) + "; found " + controlsHash + "."
                );
            }

            if (!HashEquals(proxyHash, policy.ExpectedProxySha256))
            {
                throw new InstallerException(
                    "Compatibility proxy failed verification. Expected "
                    + NormalizeHash(policy.ExpectedProxySha256) + "; found " + proxyHash + "."
                );
            }

            string controlsDestination = SafeChild(gameDirectory, ControlsRelativePath);
            if (File.Exists(controlsDestination))
            {
                AssertRegularFile(controlsDestination, "existing scripts\\TrueSwing.dll");
                string existingControlsHash = Sha256File(controlsDestination);
                if (!HashEquals(existingControlsHash, policy.ExpectedControlsSha256)
                    && !HashAllowed(existingControlsHash, policy.ReplaceableControlsSha256))
                {
                    throw new InstallerException(
                        "A different scripts\\TrueSwing.dll is already installed (SHA-256 "
                        + existingControlsHash
                        + "). It may be the separate physics mod. Refusing to overwrite it. No game files were changed."
                    );
                }
            }

            string proxyDestination = SafeChild(gameDirectory, ProxyRelativePath);
            if (File.Exists(proxyDestination))
            {
                AssertRegularFile(proxyDestination, "existing winmm.dll");
                string existingProxyHash = Sha256File(proxyDestination);
                if (!HashEquals(existingProxyHash, policy.ExpectedProxySha256)
                    && !HashAllowed(existingProxyHash, policy.ReplaceableProxySha256))
                {
                    throw new InstallerException(
                        "An unknown game-directory winmm.dll is already installed (SHA-256 "
                        + existingProxyHash
                        + "). Refusing to overwrite another loader. No game files were changed."
                    );
                }
            }

            List<DesiredFile> desiredFiles = new List<DesiredFile>();
            desiredFiles.Add(CreateDesiredFile(gameDirectory, ControlsRelativePath, controlsBytes));

            string scriptsListPath = SafeChild(gameDirectory, ScriptsListRelativePath);
            string scriptsList = ReadSmallTextFile(scriptsListPath, ScriptsListRelativePath);
            string updatedScriptsList = AddLineIfMissing(scriptsList, "TrueSwing.dll");
            desiredFiles.Add(CreateDesiredFile(
                gameDirectory,
                ScriptsListRelativePath,
                new UTF8Encoding(false).GetBytes(updatedScriptsList)
            ));

            desiredFiles.Add(CreateDesiredFile(gameDirectory, ProxyRelativePath, proxyBytes));

            string commandLinePath = SafeChild(gameDirectory, CommandLineRelativePath);
            string commandLine = ReadSmallTextFile(commandLinePath, CommandLineRelativePath);
            string updatedCommandLine = AddArgumentIfMissing(commandLine, "-scripts");
            desiredFiles.Add(CreateDesiredFile(
                gameDirectory,
                CommandLineRelativePath,
                new UTF8Encoding(false).GetBytes(updatedCommandLine)
            ));

            List<InstallAction> actions = new List<InstallAction>();
            foreach (DesiredFile desired in desiredFiles)
            {
                actions.Add(desired.Action);
            }

            PreparedPlan prepared = new PreparedPlan();
            prepared.DesiredFiles = desiredFiles;
            prepared.PublicPlan = new InstallPlan(
                resolvedToolDirectory,
                assetArchiveDirectory,
                gameDirectory,
                Sha256File(toolExecutable),
                gameHash,
                actions
            );
            return prepared;
        }

        private static InstallResult Commit(PreparedPlan prepared, InstallerPolicy policy)
        {
            DateTime now = policy.UtcNow == null ? DateTime.UtcNow : policy.UtcNow();
            string id = policy.NewId == null ? Guid.NewGuid().ToString("N") : policy.NewId();
            if (string.IsNullOrEmpty(id))
            {
                id = Guid.NewGuid().ToString("N");
            }

            if (id.Length > 8)
            {
                id = id.Substring(0, 8);
            }

            string backupRoot = SafeChild(prepared.PublicPlan.GameDirectory, BackupRootName);
            AssertSafeDirectoryIfPresent(backupRoot, "backup root");
            string backupDirectory = SafeChild(
                backupRoot,
                now.ToString("yyyyMMddHHmmssfff") + "-" + SanitizeName(id)
            );
            AssertWritablePathLengths(backupDirectory, prepared.DesiredFiles);
            if (Directory.Exists(backupDirectory) || File.Exists(backupDirectory))
            {
                throw new InstallerException("Backup target already exists: " + backupDirectory);
            }

            Directory.CreateDirectory(backupDirectory);
            List<CommitRecord> records = PrepareBackups(prepared, backupDirectory);
            WriteManifest(backupDirectory, prepared.PublicPlan, records, "prepared", null);

            List<CommitRecord> committed = new List<CommitRecord>();
            try
            {
                foreach (CommitRecord record in records)
                {
                    if (record.Desired.Action.Kind == InstallActionKind.Unchanged)
                    {
                        continue;
                    }

                    WriteAtomically(record.Desired.Destination, record.Desired.Content, backupDirectory);
                    committed.Add(record);
                    string installedHash = Sha256File(record.Desired.Destination);
                    if (!HashEquals(installedHash, record.Desired.Action.DesiredSha256))
                    {
                        throw new IOException("Post-write verification failed for " + record.Desired.RelativePath + ".");
                    }
                }

                WriteManifest(backupDirectory, prepared.PublicPlan, records, "installed", null);
                return new InstallResult(prepared.PublicPlan, true, backupDirectory);
            }
            catch (Exception ex)
            {
                List<string> rollbackErrors = RollBack(committed, backupDirectory);
                string rollbackMessage = rollbackErrors.Count == 0
                    ? "Rollback completed."
                    : "Rollback had errors: " + string.Join(" | ", rollbackErrors.ToArray());
                try
                {
                    WriteManifest(backupDirectory, prepared.PublicPlan, records, "failed", ex.Message + " " + rollbackMessage);
                }
                catch
                {
                }

                throw new InstallerException(
                    "Installation failed. " + rollbackMessage + " Backup: " + backupDirectory,
                    ex
                );
            }
        }

        private static List<CommitRecord> PrepareBackups(PreparedPlan prepared, string backupDirectory)
        {
            List<CommitRecord> records = new List<CommitRecord>();
            foreach (DesiredFile desired in prepared.DesiredFiles)
            {
                bool existed = File.Exists(desired.Destination);
                string backupPath = null;
                if (desired.Action.Kind == InstallActionKind.Replace)
                {
                    AssertRegularFile(desired.Destination, "existing " + desired.RelativePath);
                    backupPath = SafeChild(Path.Combine(backupDirectory, "o"), desired.RelativePath);
                    Directory.CreateDirectory(Path.GetDirectoryName(backupPath));
                    File.Copy(desired.Destination, backupPath, false);
                    if (!HashEquals(Sha256File(backupPath), desired.Action.ExistingSha256))
                    {
                        throw new InstallerException("Backup verification failed for " + desired.RelativePath + ".");
                    }
                }

                CommitRecord record = new CommitRecord();
                record.Desired = desired;
                record.Existed = existed;
                record.BackupPath = backupPath;
                records.Add(record);
            }

            return records;
        }

        private static List<string> RollBack(IList<CommitRecord> committed, string backupDirectory)
        {
            List<string> errors = new List<string>();
            for (int index = committed.Count - 1; index >= 0; index--)
            {
                CommitRecord record = committed[index];
                try
                {
                    if (record.Existed)
                    {
                        if (string.IsNullOrEmpty(record.BackupPath) || !File.Exists(record.BackupPath))
                        {
                            throw new IOException("Verified backup is missing.");
                        }

                        WriteAtomically(record.Desired.Destination, File.ReadAllBytes(record.BackupPath), backupDirectory);
                    }
                    else if (File.Exists(record.Desired.Destination))
                    {
                        string currentHash = Sha256File(record.Desired.Destination);
                        if (!HashEquals(currentHash, record.Desired.Action.DesiredSha256))
                        {
                            throw new IOException("Created target changed externally and was preserved.");
                        }

                        string preserved = SafeChild(
                            Path.Combine(backupDirectory, "r"),
                            record.Desired.RelativePath
                        );
                        Directory.CreateDirectory(Path.GetDirectoryName(preserved));
                        File.Move(record.Desired.Destination, preserved);
                    }
                }
                catch (Exception ex)
                {
                    errors.Add(record.Desired.RelativePath + ": " + ex.Message);
                }
            }

            return errors;
        }

        private static void WriteManifest(
            string backupDirectory,
            InstallPlan plan,
            IList<CommitRecord> records,
            string status,
            string error)
        {
            StringBuilder text = new StringBuilder();
            text.AppendLine("Web Swing Controls SMPCTool Companion install manifest");
            text.AppendLine("Status=" + status);
            text.AppendLine("GameDirectory=" + plan.GameDirectory);
            text.AppendLine("SMPCToolSHA256=" + plan.SMPCToolSha256);
            text.AppendLine("SpiderManExeSHA256=" + plan.GameExecutableSha256);
            if (!string.IsNullOrEmpty(error))
            {
                text.AppendLine("Error=" + error.Replace("\r", " ").Replace("\n", " "));
            }

            foreach (CommitRecord record in records)
            {
                text.AppendLine(
                    "File=" + record.Desired.RelativePath
                    + "|Action=" + record.Desired.Action.Kind
                    + "|OriginalSHA256=" + (record.Desired.Action.ExistingSha256 ?? "")
                    + "|InstalledSHA256=" + record.Desired.Action.DesiredSha256
                    + "|Backup=" + (record.BackupPath ?? "")
                );
            }

            File.WriteAllText(
                Path.Combine(backupDirectory, "install-manifest.txt"),
                text.ToString(),
                new UTF8Encoding(false)
            );
        }

        private static void WriteAtomically(string destination, byte[] content, string backupDirectory)
        {
            string parent = Path.GetDirectoryName(destination);
            if (string.IsNullOrEmpty(parent))
            {
                throw new IOException("Could not resolve target folder for " + destination + ".");
            }

            Directory.CreateDirectory(parent);
            string stagingDirectory = Path.Combine(backupDirectory, "s");
            Directory.CreateDirectory(stagingDirectory);
            string stagingPath = Path.Combine(stagingDirectory, Guid.NewGuid().ToString("N") + ".tmp");
            File.WriteAllBytes(stagingPath, content);
            if (!HashEquals(Sha256File(stagingPath), Sha256Bytes(content)))
            {
                throw new IOException("Staged-file verification failed for " + destination + ".");
            }

            if (File.Exists(destination))
            {
                AssertRegularFile(destination, "replacement target");
                File.Replace(stagingPath, destination, null, true);
            }
            else
            {
                File.Move(stagingPath, destination);
            }
        }

        private static DesiredFile CreateDesiredFile(string gameDirectory, string relativePath, byte[] content)
        {
            string destination = SafeChild(gameDirectory, relativePath);
            string desiredHash = Sha256Bytes(content);
            string existingHash = null;
            InstallActionKind kind = InstallActionKind.Create;
            if (File.Exists(destination))
            {
                AssertRegularFile(destination, "existing " + relativePath);
                existingHash = Sha256File(destination);
                kind = HashEquals(existingHash, desiredHash)
                    ? InstallActionKind.Unchanged
                    : InstallActionKind.Replace;
            }
            else if (Directory.Exists(destination))
            {
                throw new InstallerException("Install target is a directory: " + destination);
            }

            DesiredFile desired = new DesiredFile();
            desired.RelativePath = relativePath;
            desired.Destination = destination;
            desired.Content = content;
            desired.Action = new InstallAction(relativePath, kind, existingHash, desiredHash);
            return desired;
        }

        private static void AssertWritablePathLengths(string backupDirectory, IList<DesiredFile> desiredFiles)
        {
            if (backupDirectory.Length > LegacyMaximumDirectoryPathCharacters)
            {
                ThrowPathTooLong();
            }

            List<string> candidates = new List<string>();
            candidates.Add(Path.Combine(backupDirectory, "install-manifest.txt"));
            candidates.Add(Path.Combine(backupDirectory, "s", new string('0', 32) + ".tmp"));

            foreach (DesiredFile desired in desiredFiles)
            {
                candidates.Add(desired.Destination);
                candidates.Add(SafeChild(Path.Combine(backupDirectory, "o"), desired.RelativePath));
                candidates.Add(SafeChild(Path.Combine(backupDirectory, "r"), desired.RelativePath));
            }

            foreach (string candidate in candidates)
            {
                if (candidate.Length > LegacyMaximumPathCharacters)
                {
                    ThrowPathTooLong();
                }
            }
        }

        private static void ThrowPathTooLong()
        {
            throw new InstallerException(
                "Game folder path is too long for this .NET Framework companion. "
                + "Move the game or SMPCTool to a shorter path, then retry. No game files were changed."
            );
        }

        private static string ReadSmallTextFile(string path, string label)
        {
            if (!File.Exists(path))
            {
                return string.Empty;
            }

            AssertRegularFile(path, label);
            if (new FileInfo(path).Length > MaximumTextFileBytes)
            {
                throw new InstallerException(label + " is unexpectedly large; refusing to rewrite it.");
            }

            return File.ReadAllText(path);
        }

        private static string AddLineIfMissing(string existing, string lineToAdd)
        {
            string[] lines = existing.Split(new char[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries);
            foreach (string line in lines)
            {
                if (string.Equals(line.Trim(), lineToAdd, StringComparison.OrdinalIgnoreCase))
                {
                    return existing;
                }
            }

            if (string.IsNullOrWhiteSpace(existing))
            {
                return lineToAdd;
            }

            string separator = existing.EndsWith("\r", StringComparison.Ordinal)
                || existing.EndsWith("\n", StringComparison.Ordinal)
                ? string.Empty
                : Environment.NewLine;
            return existing + separator + lineToAdd;
        }

        private static string AddArgumentIfMissing(string existing, string argument)
        {
            string[] tokens = existing.Split((char[])null, StringSplitOptions.RemoveEmptyEntries);
            foreach (string token in tokens)
            {
                if (string.Equals(token, argument, StringComparison.OrdinalIgnoreCase))
                {
                    return existing;
                }
            }

            if (existing.Length == 0)
            {
                return argument;
            }

            string separator = char.IsWhiteSpace(existing[existing.Length - 1]) ? string.Empty : " ";
            return existing + separator + argument;
        }

        private static void ValidatePolicy(InstallerPolicy policy)
        {
            ValidateHash(policy.ExpectedGameExecutableSha256, "ExpectedGameExecutableSha256");
            ValidateHash(policy.ExpectedProxySha256, "ExpectedProxySha256");
            ValidateHash(policy.ExpectedControlsSha256, "ExpectedControlsSha256");
            foreach (string hash in policy.ReplaceableProxySha256)
            {
                ValidateHash(hash, "ReplaceableProxySha256");
            }

            foreach (string hash in policy.ReplaceableControlsSha256)
            {
                ValidateHash(hash, "ReplaceableControlsSha256");
            }
        }

        private static void ValidateHash(string hash, string label)
        {
            string normalized = NormalizeHash(hash);
            if (normalized.Length != 64)
            {
                throw new InstallerException(label + " must be a SHA-256 value.");
            }

            for (int index = 0; index < normalized.Length; index++)
            {
                char value = normalized[index];
                if (!((value >= '0' && value <= '9') || (value >= 'A' && value <= 'F')))
                {
                    throw new InstallerException(label + " must be a SHA-256 value.");
                }
            }
        }

        private static bool HashAllowed(string hash, IList<string> allowed)
        {
            foreach (string candidate in allowed)
            {
                if (HashEquals(hash, candidate))
                {
                    return true;
                }
            }

            return false;
        }

        private static bool HashEquals(string left, string right)
        {
            return string.Equals(NormalizeHash(left), NormalizeHash(right), StringComparison.OrdinalIgnoreCase);
        }

        private static string NormalizeHash(string hash)
        {
            return (hash ?? string.Empty).Replace(" ", string.Empty).Replace("-", string.Empty).ToUpperInvariant();
        }

        public static string Sha256File(string path)
        {
            using (FileStream stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read))
            using (SHA256 sha = SHA256.Create())
            {
                return ToHex(sha.ComputeHash(stream));
            }
        }

        public static string Sha256Bytes(byte[] content)
        {
            using (SHA256 sha = SHA256.Create())
            {
                return ToHex(sha.ComputeHash(content));
            }
        }

        private static string ToHex(byte[] bytes)
        {
            StringBuilder result = new StringBuilder(bytes.Length * 2);
            foreach (byte value in bytes)
            {
                result.Append(value.ToString("X2"));
            }

            return result.ToString();
        }

        private static string ResolveDirectory(string path, string label)
        {
            if (string.IsNullOrWhiteSpace(path))
            {
                throw new InstallerException(label + " is not set.");
            }

            string fullPath = Path.GetFullPath(path).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
            if (!Directory.Exists(fullPath))
            {
                throw new InstallerException(label + " does not exist: " + fullPath);
            }

            AssertSafeDirectoryIfPresent(fullPath, label);
            string root = Path.GetPathRoot(fullPath).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
            if (string.Equals(fullPath, root, StringComparison.OrdinalIgnoreCase))
            {
                throw new InstallerException("A drive root is not a valid " + label + ".");
            }

            return fullPath;
        }

        private static string SafeChild(string root, string relativePath)
        {
            string fullRoot = Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
            string fullPath = Path.GetFullPath(Path.Combine(fullRoot, relativePath));
            string prefix = fullRoot + Path.DirectorySeparatorChar;
            if (!fullPath.StartsWith(prefix, StringComparison.OrdinalIgnoreCase))
            {
                throw new InstallerException("Path escapes its allowed folder: " + relativePath);
            }

            return fullPath;
        }

        private static void AssertRegularFile(string path, string label)
        {
            if (!File.Exists(path))
            {
                throw new InstallerException(label + " was not found: " + path);
            }

            FileAttributes attributes = File.GetAttributes(path);
            if ((attributes & FileAttributes.ReparsePoint) != 0)
            {
                throw new InstallerException(label + " is a link or reparse point; refusing it.");
            }
        }

        private static void AssertSafeDirectoryIfPresent(string path, string label)
        {
            if (!Directory.Exists(path))
            {
                return;
            }

            FileAttributes attributes = File.GetAttributes(path);
            if ((attributes & FileAttributes.ReparsePoint) != 0)
            {
                throw new InstallerException(label + " is a link or reparse point; refusing it.");
            }
        }

        private static string SanitizeName(string value)
        {
            StringBuilder result = new StringBuilder();
            foreach (char character in value)
            {
                if (char.IsLetterOrDigit(character) || character == '-' || character == '_')
                {
                    result.Append(character);
                }
            }

            return result.Length == 0 ? "install" : result.ToString();
        }
    }
}

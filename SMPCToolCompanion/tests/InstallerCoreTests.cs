using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using WebSwingControls.SMPCToolCompanion;

internal static class InstallerCoreTests
{
    private static readonly byte[] GameBytes = Encoding.ASCII.GetBytes("supported-game-fixture");
    private static readonly byte[] ControlsBytes = Encoding.ASCII.GetBytes("web-swing-controls-current");
    private static readonly byte[] OldControlsBytes = Encoding.ASCII.GetBytes("web-swing-controls-old");
    private static readonly byte[] ProxyBytes = Encoding.ASCII.GetBytes("host-gated-proxy-current");
    private static readonly byte[] StockProxyBytes = Encoding.ASCII.GetBytes("known-stock-proxy");

    private sealed class Fixture
    {
        public string Root;
        public string ToolDirectory;
        public string GameDirectory;
        public string AssetArchiveDirectory;
        public string PayloadDirectory;
        public InstallerPolicy Policy;
    }

    private static int Main(string[] args)
    {
        if (args.Length != 1)
        {
            Console.Error.WriteLine("Usage: InstallerCoreTests.exe <new-output-directory>");
            return 2;
        }

        string root = Path.GetFullPath(args[0]);
        if (Directory.Exists(root) || File.Exists(root))
        {
            Console.Error.WriteLine("Refusing existing test output: " + root);
            return 3;
        }

        Directory.CreateDirectory(root);
        int passed = 0;
        passed += Run("fresh install and idempotence", delegate { FreshInstallAndIdempotence(root); });
        passed += Run("existing settings preserved", delegate { ExistingSettingsPreserved(root); });
        passed += Run("unknown proxy refused without mutation", delegate { UnknownProxyRefused(root); });
        passed += Run("conflicting TrueSwing refused", delegate { ConflictingControlsRefused(root); });
        passed += Run("unsupported game refused", delegate { UnsupportedGameRefused(root); });
        passed += Run("running-process guard does not write", delegate { RunningProcessGuard(root); });
        passed += Run("known upgrades receive verified backups", delegate { KnownUpgradeBackups(root); });
        passed += Run("wrong asset folder refused", delegate { WrongAssetFolderRefused(root); });
        passed += Run("tampered payload refused", delegate { TamperedPayloadRefused(root); });
        passed += Run("missing SMPCTool refused", delegate { MissingToolRefused(root); });
        passed += Run("tool discovery supports adjacent folder", delegate { AdjacentToolDiscovery(root); });
        passed += Run("long install path refused without mutation", delegate { LongPathRefused(root); });

        Console.WriteLine("SMPCTOOL_COMPANION_TESTS_OK cases=" + passed + " artifacts=" + root);
        return passed == 12 ? 0 : 1;
    }

    private static int Run(string name, Action test)
    {
        try
        {
            test();
            Console.WriteLine("PASS " + name);
            return 1;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine("FAIL " + name + ": " + ex);
            Environment.ExitCode = 1;
            return 0;
        }
    }

    private static void FreshInstallAndIdempotence(string root)
    {
        Fixture fixture = CreateFixture(root, "01-fresh");
        InstallerCore installer = new InstallerCore();
        InstallPlan plan = installer.Inspect(fixture.ToolDirectory, fixture.PayloadDirectory, fixture.Policy);
        Assert(plan.Actions.Count == 4, "Expected four planned files.");
        Assert(plan.Actions.All(action => action.Kind == InstallActionKind.Create), "Fresh plan should create every file.");

        InstallResult result = installer.Install(fixture.ToolDirectory, fixture.PayloadDirectory, fixture.Policy);
        Assert(result.Changed, "Fresh install should report changes.");
        AssertBytes(Path.Combine(fixture.GameDirectory, "scripts", "TrueSwing.dll"), ControlsBytes);
        AssertBytes(Path.Combine(fixture.GameDirectory, "winmm.dll"), ProxyBytes);
        Assert(File.ReadAllText(Path.Combine(fixture.GameDirectory, "scripts.txt")) == "TrueSwing.dll", "Unexpected scripts.txt.");
        Assert(File.ReadAllText(Path.Combine(fixture.GameDirectory, "commandline.txt")) == "-scripts", "Unexpected commandline.txt.");
        Assert(File.Exists(Path.Combine(result.BackupDirectory, "install-manifest.txt")), "Manifest missing.");

        InstallPlan secondPlan = installer.Inspect(fixture.ToolDirectory, fixture.PayloadDirectory, fixture.Policy);
        Assert(!secondPlan.HasChanges, "Second plan should be idempotent.");
        InstallResult secondResult = installer.Install(fixture.ToolDirectory, fixture.PayloadDirectory, fixture.Policy);
        Assert(!secondResult.Changed, "Second install should not write.");
        Assert(secondResult.BackupDirectory == null, "Idempotent install should not create backup.");
    }

    private static void ExistingSettingsPreserved(string root)
    {
        Fixture fixture = CreateFixture(root, "02-preserve");
        File.WriteAllText(Path.Combine(fixture.GameDirectory, "scripts.txt"), "AnotherMod.dll\r\n", new UTF8Encoding(false));
        File.WriteAllText(Path.Combine(fixture.GameDirectory, "commandline.txt"), "-windowed -foo", new UTF8Encoding(false));

        new InstallerCore().Install(fixture.ToolDirectory, fixture.PayloadDirectory, fixture.Policy);
        string scripts = File.ReadAllText(Path.Combine(fixture.GameDirectory, "scripts.txt"));
        string commandLine = File.ReadAllText(Path.Combine(fixture.GameDirectory, "commandline.txt"));
        Assert(scripts.Contains("AnotherMod.dll"), "Existing script entry lost.");
        Assert(scripts.Contains("TrueSwing.dll"), "Controls entry missing.");
        Assert(commandLine.Contains("-windowed -foo"), "Existing command-line options lost.");
        Assert(commandLine.Contains("-scripts"), "-scripts missing.");
    }

    private static void UnknownProxyRefused(string root)
    {
        Fixture fixture = CreateFixture(root, "03-unknown-proxy");
        File.WriteAllBytes(Path.Combine(fixture.GameDirectory, "winmm.dll"), Encoding.ASCII.GetBytes("unknown-loader"));
        string before = Snapshot(fixture.Root);
        AssertThrows(
            delegate { new InstallerCore().Inspect(fixture.ToolDirectory, fixture.PayloadDirectory, fixture.Policy); },
            "unknown game-directory winmm.dll"
        );
        Assert(before == Snapshot(fixture.Root), "Unknown-proxy refusal mutated files.");
    }

    private static void ConflictingControlsRefused(string root)
    {
        Fixture fixture = CreateFixture(root, "04-controls-conflict");
        Directory.CreateDirectory(Path.Combine(fixture.GameDirectory, "scripts"));
        File.WriteAllBytes(
            Path.Combine(fixture.GameDirectory, "scripts", "TrueSwing.dll"),
            Encoding.ASCII.GetBytes("different-physics-mod")
        );
        string before = Snapshot(fixture.Root);
        AssertThrows(
            delegate { new InstallerCore().Inspect(fixture.ToolDirectory, fixture.PayloadDirectory, fixture.Policy); },
            "different scripts\\TrueSwing.dll"
        );
        Assert(before == Snapshot(fixture.Root), "Controls-conflict refusal mutated files.");
    }

    private static void UnsupportedGameRefused(string root)
    {
        Fixture fixture = CreateFixture(root, "05-unsupported-game");
        File.WriteAllBytes(Path.Combine(fixture.GameDirectory, "Spider-Man.exe"), Encoding.ASCII.GetBytes("other-build"));
        string before = Snapshot(fixture.Root);
        AssertThrows(
            delegate { new InstallerCore().Inspect(fixture.ToolDirectory, fixture.PayloadDirectory, fixture.Policy); },
            "Unsupported Spider-Man.exe build"
        );
        Assert(before == Snapshot(fixture.Root), "Unsupported-game refusal mutated files.");
    }

    private static void RunningProcessGuard(string root)
    {
        Fixture fixture = CreateFixture(root, "06-running");
        fixture.Policy.IsProtectedProcessRunning = delegate { return true; };
        string before = Snapshot(fixture.Root);
        AssertThrows(
            delegate { new InstallerCore().Install(fixture.ToolDirectory, fixture.PayloadDirectory, fixture.Policy); },
            "never closes processes"
        );
        Assert(before == Snapshot(fixture.Root), "Running-process refusal mutated files.");
    }

    private static void KnownUpgradeBackups(string root)
    {
        Fixture fixture = CreateFixture(root, "07-upgrade");
        Directory.CreateDirectory(Path.Combine(fixture.GameDirectory, "scripts"));
        string oldControlsPath = Path.Combine(fixture.GameDirectory, "scripts", "TrueSwing.dll");
        string oldProxyPath = Path.Combine(fixture.GameDirectory, "winmm.dll");
        File.WriteAllBytes(oldControlsPath, OldControlsBytes);
        File.WriteAllBytes(oldProxyPath, StockProxyBytes);
        fixture.Policy.ReplaceableControlsSha256.Add(InstallerCore.Sha256Bytes(OldControlsBytes));
        fixture.Policy.ReplaceableProxySha256.Add(InstallerCore.Sha256Bytes(StockProxyBytes));

        InstallResult result = new InstallerCore().Install(fixture.ToolDirectory, fixture.PayloadDirectory, fixture.Policy);
        string controlsBackup = Path.Combine(result.BackupDirectory, "o", "scripts", "TrueSwing.dll");
        string proxyBackup = Path.Combine(result.BackupDirectory, "o", "winmm.dll");
        AssertBytes(controlsBackup, OldControlsBytes);
        AssertBytes(proxyBackup, StockProxyBytes);
        AssertBytes(oldControlsPath, ControlsBytes);
        AssertBytes(oldProxyPath, ProxyBytes);
        string manifest = File.ReadAllText(Path.Combine(result.BackupDirectory, "install-manifest.txt"));
        Assert(manifest.Contains("Status=installed"), "Installed manifest status missing.");
    }

    private static void WrongAssetFolderRefused(string root)
    {
        Fixture fixture = CreateFixture(root, "08-wrong-asset");
        string wrong = Path.Combine(fixture.GameDirectory, "not_asset_archive");
        Directory.CreateDirectory(wrong);
        File.WriteAllBytes(Path.Combine(wrong, "toc"), new byte[] { 1 });
        File.WriteAllText(Path.Combine(fixture.ToolDirectory, "assetArchiveDir.txt"), wrong, new UTF8Encoding(false));
        string before = Snapshot(fixture.Root);
        AssertThrows(
            delegate { new InstallerCore().Inspect(fixture.ToolDirectory, fixture.PayloadDirectory, fixture.Policy); },
            "does not point to the game's asset_archive folder"
        );
        Assert(before == Snapshot(fixture.Root), "Wrong-folder refusal mutated files.");
    }

    private static void TamperedPayloadRefused(string root)
    {
        Fixture fixture = CreateFixture(root, "09-tampered-payload");
        File.WriteAllBytes(Path.Combine(fixture.PayloadDirectory, "winmm.dll"), Encoding.ASCII.GetBytes("tampered"));
        string before = Snapshot(fixture.Root);
        AssertThrows(
            delegate { new InstallerCore().Inspect(fixture.ToolDirectory, fixture.PayloadDirectory, fixture.Policy); },
            "Compatibility proxy failed verification"
        );
        Assert(before == Snapshot(fixture.Root), "Payload refusal mutated files.");
    }

    private static void MissingToolRefused(string root)
    {
        Fixture fixture = CreateFixture(root, "10-missing-tool");
        string missingToolDirectory = Path.Combine(fixture.Root, "empty-tool-folder");
        Directory.CreateDirectory(missingToolDirectory);
        string before = Snapshot(fixture.Root);
        AssertThrows(
            delegate { new InstallerCore().Inspect(missingToolDirectory, fixture.PayloadDirectory, fixture.Policy); },
            "SMPCTool.exe was not found"
        );
        Assert(before == Snapshot(fixture.Root), "Missing-tool refusal mutated files.");
    }

    private static void AdjacentToolDiscovery(string root)
    {
        Fixture fixture = CreateFixture(root, "11-discovery");
        string companionDirectory = Path.Combine(fixture.ToolDirectory, "Web Swing Controls Companion");
        Directory.CreateDirectory(companionDirectory);

        Assert(
            InstallerCore.LocateToolDirectory(companionDirectory) == fixture.ToolDirectory,
            "Companion folder should find SMPCTool.exe in its parent folder."
        );
        Assert(
            InstallerCore.LocateToolDirectory(fixture.ToolDirectory) == fixture.ToolDirectory,
            "Companion contents placed directly beside SMPCTool.exe should use the same folder."
        );
    }

    private static void LongPathRefused(string root)
    {
        const string gameFolderName = "Marvel's Spider-Man Remastered";
        int nameLength = 210 - root.Length - gameFolderName.Length - 2;
        Assert(nameLength >= 20, "Test output root is too long for long-path fixture setup.");
        Fixture fixture = CreateFixture(root, new string('l', nameLength));
        Assert(fixture.GameDirectory.Length == 210, "Long-path fixture length changed unexpectedly.");
        string before = Snapshot(fixture.Root);

        AssertThrows(
            delegate { new InstallerCore().Install(fixture.ToolDirectory, fixture.PayloadDirectory, fixture.Policy); },
            "Game folder path is too long"
        );
        Assert(before == Snapshot(fixture.Root), "Long-path refusal mutated files.");
    }

    private static Fixture CreateFixture(string root, string name)
    {
        Fixture fixture = new Fixture();
        fixture.Root = Path.Combine(root, name);
        fixture.ToolDirectory = Path.Combine(fixture.Root, "SMPCTool");
        fixture.GameDirectory = Path.Combine(fixture.Root, "Marvel's Spider-Man Remastered");
        fixture.AssetArchiveDirectory = Path.Combine(fixture.GameDirectory, "asset_archive");
        fixture.PayloadDirectory = Path.Combine(fixture.ToolDirectory, "payload");

        Directory.CreateDirectory(fixture.ToolDirectory);
        Directory.CreateDirectory(fixture.AssetArchiveDirectory);
        Directory.CreateDirectory(fixture.PayloadDirectory);
        File.WriteAllBytes(Path.Combine(fixture.ToolDirectory, "SMPCTool.exe"), Encoding.ASCII.GetBytes("official-tool-fixture"));
        File.WriteAllText(
            Path.Combine(fixture.ToolDirectory, "assetArchiveDir.txt"),
            fixture.AssetArchiveDirectory + Path.DirectorySeparatorChar,
            new UTF8Encoding(false)
        );
        File.WriteAllBytes(Path.Combine(fixture.AssetArchiveDirectory, "toc"), new byte[] { 1, 2, 3 });
        File.WriteAllBytes(Path.Combine(fixture.GameDirectory, "Spider-Man.exe"), GameBytes);
        File.WriteAllBytes(Path.Combine(fixture.PayloadDirectory, "TrueSwing.dll"), ControlsBytes);
        File.WriteAllBytes(Path.Combine(fixture.PayloadDirectory, "winmm.dll"), ProxyBytes);

        fixture.Policy = new InstallerPolicy();
        fixture.Policy.ExpectedGameExecutableSha256 = InstallerCore.Sha256Bytes(GameBytes);
        fixture.Policy.ExpectedControlsSha256 = InstallerCore.Sha256Bytes(ControlsBytes);
        fixture.Policy.ExpectedProxySha256 = InstallerCore.Sha256Bytes(ProxyBytes);
        fixture.Policy.IsProtectedProcessRunning = delegate { return false; };
        fixture.Policy.UtcNow = delegate { return new DateTime(2026, 9, 1, 12, 0, 0, DateTimeKind.Utc); };
        fixture.Policy.NewId = delegate { return name.Replace("-", string.Empty); };
        return fixture;
    }

    private static string Snapshot(string root)
    {
        StringBuilder result = new StringBuilder();
        foreach (string file in Directory.GetFiles(root, "*", SearchOption.AllDirectories).OrderBy(path => path, StringComparer.OrdinalIgnoreCase))
        {
            result.Append(file.Substring(root.Length).Replace('\\', '/'));
            result.Append('|');
            result.Append(InstallerCore.Sha256File(file));
            result.AppendLine();
        }

        return result.ToString();
    }

    private static void AssertBytes(string path, byte[] expected)
    {
        Assert(File.Exists(path), "Missing file: " + path);
        Assert(File.ReadAllBytes(path).SequenceEqual(expected), "Unexpected bytes: " + path);
    }

    private static void AssertThrows(Action action, string expectedText)
    {
        try
        {
            action();
        }
        catch (Exception ex)
        {
            Assert(ex.Message.IndexOf(expectedText, StringComparison.OrdinalIgnoreCase) >= 0,
                "Unexpected error. Expected '" + expectedText + "'; got '" + ex.Message + "'.");
            return;
        }

        throw new InvalidOperationException("Expected exception containing: " + expectedText);
    }

    private static void Assert(bool condition, string message)
    {
        if (!condition)
        {
            throw new InvalidOperationException(message);
        }
    }
}

using System.IO.Compression;
using System.Security.Cryptography;
using System.Text;
using ModdingToolScriptSupportCore;

internal static class Program {
	private static int _passed;
	private static readonly byte[] ProxyBytes = Enumerable.Range(0, 256).Select(value => (byte)value).ToArray();
	private static readonly byte[] StockProxyBytes = Enumerable.Range(0, 256).Select(value => (byte)(255 - value)).ToArray();
	private static readonly byte[] ScriptDllBytes = Encoding.ASCII.GetBytes("VALID_TEST_SCRIPT_DLL_V021");

	private static int Main(string[] args) {
		var outputBase = args.Length > 0
			? Path.GetFullPath(args[0])
			: Path.GetFullPath(Path.Combine("test-output", "run-" + DateTime.UtcNow.ToString("yyyyMMdd-HHmmssfff")));
		Directory.CreateDirectory(outputBase);

		try {
			Run("fresh install and idempotence", () => TestFreshInstall(Path.Combine(outputBase, "01-fresh")));
			Run("existing scripts and arguments preserved", () => TestPreservesExistingState(Path.Combine(outputBase, "02-preserve")));
			Run("unknown proxy refused without mutation", () => TestUnknownProxyRefusal(Path.Combine(outputBase, "03-unknown-proxy")));
			Run("unsupported game executable refused", () => TestUnsupportedExecutable(Path.Combine(outputBase, "04-wrong-game")));
			Run("zip traversal refused", () => TestTraversalRefusal(Path.Combine(outputBase, "05-traversal")));
			Run("dependencies refused", () => TestDependencyRefusal(Path.Combine(outputBase, "06-dependency")));
			Run("running game guard does not close or write", () => TestRunningProcessGuard(Path.Combine(outputBase, "07-running")));
			Run("script collision receives verified backup", () => TestScriptCollisionBackup(Path.Combine(outputBase, "08-collision")));

			Console.WriteLine($"SCRIPT_ADDON_TESTS_OK cases={_passed} artifacts={outputBase}");
			return 0;
		} catch (Exception ex) {
			Console.Error.WriteLine("SCRIPT_ADDON_TESTS_FAILED");
			Console.Error.WriteLine(ex);
			Console.Error.WriteLine($"Artifacts preserved at: {outputBase}");
			return 1;
		}
	}

	private static void TestFreshInstall(string root) {
		var fixture = CreateFixture(root);
		var installer = new ScriptPackageInstaller();
		var plan = installer.Inspect(fixture.Options);
		Assert(plan.HasChanges, "Fresh plan should have changes.");
		AssertEqual(4, plan.CreateCount, "Fresh create count");
		AssertEqual(0, plan.ReplaceCount, "Fresh replace count");

		var result = installer.Install(fixture.Options);
		Assert(result.Changed, "Fresh install should report changes.");
		Assert(result.BackupDirectory != null && Directory.Exists(result.BackupDirectory), "Fresh install manifest directory missing.");
		AssertBytes(ScriptDllBytes, File.ReadAllBytes(Path.Combine(fixture.Game, "scripts", "TrueSwing.dll")), "Installed script DLL");
		AssertBytes(ProxyBytes, File.ReadAllBytes(Path.Combine(fixture.Game, "winmm.dll")), "Installed proxy");
		AssertEqual("TrueSwing.dll", File.ReadAllText(Path.Combine(fixture.Game, "scripts.txt")), "scripts.txt");
		AssertEqual("-scripts", File.ReadAllText(Path.Combine(fixture.Game, "commandline.txt")), "commandline.txt");
		Assert(File.Exists(Path.Combine(result.BackupDirectory!, "install-manifest.json")), "Install manifest missing.");

		var second = installer.Install(fixture.Options);
		Assert(!second.Changed, "Second install should be idempotent.");
		Assert(second.BackupDirectory == null, "Idempotent install should not create another backup.");
		Assert(!installer.Inspect(fixture.Options).HasChanges, "Post-install inspection should be clean.");
	}

	private static void TestPreservesExistingState(string root) {
		var fixture = CreateFixture(root);
		var scripts = Path.Combine(fixture.Game, "scripts");
		Directory.CreateDirectory(scripts);
		var otherDll = Encoding.ASCII.GetBytes("OTHER_SCRIPT");
		File.WriteAllBytes(Path.Combine(scripts, "Other.dll"), otherDll);
		File.WriteAllText(Path.Combine(fixture.Game, "scripts.txt"), "Other.dll  ");
		File.WriteAllText(Path.Combine(fixture.Game, "commandline.txt"), "-skipStartScreen -windowed");
		File.WriteAllBytes(Path.Combine(fixture.Game, "winmm.dll"), StockProxyBytes);

		var installer = new ScriptPackageInstaller();
		var result = installer.Install(fixture.Options);
		Assert(result.Changed, "Preservation fixture should change.");
		AssertBytes(otherDll, File.ReadAllBytes(Path.Combine(scripts, "Other.dll")), "Existing script preserved");
		var scriptLines = File.ReadAllLines(Path.Combine(fixture.Game, "scripts.txt"));
		Assert(scriptLines.Any(line => line.Trim().Equals("Other.dll", StringComparison.OrdinalIgnoreCase)), "Other.dll missing from scripts.txt.");
		Assert(scriptLines.Any(line => line.Trim().Equals("TrueSwing.dll", StringComparison.OrdinalIgnoreCase)), "TrueSwing.dll missing from scripts.txt.");
		var commandLine = File.ReadAllText(Path.Combine(fixture.Game, "commandline.txt"));
		Assert(commandLine.Contains("-skipStartScreen"), "Existing command-line flag lost.");
		Assert(commandLine.Contains("-windowed"), "Existing command-line flag lost.");
		Assert(commandLine.Split((char[]?)null, StringSplitOptions.RemoveEmptyEntries).Count(token => token.Equals("-scripts", StringComparison.OrdinalIgnoreCase)) == 1, "-scripts should appear exactly once.");
		AssertBytes(ProxyBytes, File.ReadAllBytes(Path.Combine(fixture.Game, "winmm.dll")), "Stock proxy replacement");

		var originals = Path.Combine(result.BackupDirectory!, "originals");
		AssertBytes(StockProxyBytes, File.ReadAllBytes(Path.Combine(originals, "winmm.dll")), "Proxy backup");
		AssertEqual("Other.dll  ", File.ReadAllText(Path.Combine(originals, "scripts.txt")), "scripts.txt backup");
		AssertEqual("-skipStartScreen -windowed", File.ReadAllText(Path.Combine(originals, "commandline.txt")), "commandline backup");
	}

	private static void TestUnknownProxyRefusal(string root) {
		var fixture = CreateFixture(root);
		var unknown = Encoding.ASCII.GetBytes("UNKNOWN_LOADER");
		File.WriteAllBytes(Path.Combine(fixture.Game, "winmm.dll"), unknown);
		var before = Snapshot(fixture.Game);

		AssertThrows<ScriptInstallException>(
			() => new ScriptPackageInstaller().Inspect(fixture.Options),
			"unknown winmm.dll"
		);
		AssertSnapshotsEqual(before, Snapshot(fixture.Game), "Unknown proxy refusal mutated the fixture.");
	}

	private static void TestUnsupportedExecutable(string root) {
		var fixture = CreateFixture(root);
		var options = NewOptions(fixture.Game, fixture.Package, new string('A', 64));
		var before = Snapshot(fixture.Game);
		AssertThrows<ScriptInstallException>(
			() => new ScriptPackageInstaller().Inspect(options),
			"Unsupported Spider-Man.exe build"
		);
		AssertSnapshotsEqual(before, Snapshot(fixture.Game), "Unsupported game refusal mutated the fixture.");
	}

	private static void TestTraversalRefusal(string root) {
		var fixture = CreateFixture(root, packageFactory: path => CreatePackage(path, extraEntry: "../escape.dll"));
		var before = Snapshot(fixture.Game);
		AssertThrows<ScriptInstallException>(
			() => new ScriptPackageInstaller().Inspect(fixture.Options),
			"Unsafe archive path"
		);
		AssertSnapshotsEqual(before, Snapshot(fixture.Game), "Traversal refusal mutated the fixture.");
	}

	private static void TestDependencyRefusal(string root) {
		var fixture = CreateFixture(root, packageFactory: path => CreatePackage(path, dependencies: new[] { "ExampleLibrary:>=1.0.0" }));
		var before = Snapshot(fixture.Game);
		AssertThrows<ScriptInstallException>(
			() => new ScriptPackageInstaller().Inspect(fixture.Options),
			"dependency-free"
		);
		AssertSnapshotsEqual(before, Snapshot(fixture.Game), "Dependency refusal mutated the fixture.");
	}

	private static void TestRunningProcessGuard(string root) {
		var fixture = CreateFixture(root);
		var guardedOptions = NewOptions(fixture.Game, fixture.Package, fixture.GameHash, () => true);
		var before = Snapshot(fixture.Game);
		AssertThrows<ScriptInstallException>(
			() => new ScriptPackageInstaller().Install(guardedOptions),
			"will never close it automatically"
		);
		AssertSnapshotsEqual(before, Snapshot(fixture.Game), "Running-process guard mutated the fixture.");
	}

	private static void TestScriptCollisionBackup(string root) {
		var fixture = CreateFixture(root);
		var scripts = Path.Combine(fixture.Game, "scripts");
		Directory.CreateDirectory(scripts);
		var priorDll = Encoding.ASCII.GetBytes("PRIOR_TRUE_SWING_DLL");
		File.WriteAllBytes(Path.Combine(scripts, "TrueSwing.dll"), priorDll);

		var result = new ScriptPackageInstaller().Install(fixture.Options);
		AssertBytes(ScriptDllBytes, File.ReadAllBytes(Path.Combine(scripts, "TrueSwing.dll")), "Collision replacement");
		AssertBytes(priorDll, File.ReadAllBytes(Path.Combine(result.BackupDirectory!, "originals", "scripts", "TrueSwing.dll")), "Collision backup");
	}

	private static Fixture CreateFixture(string root, Action<string>? packageFactory = null) {
		var game = Path.Combine(root, "game");
		Directory.CreateDirectory(game);
		var gameBytes = Encoding.ASCII.GetBytes("SUPPORTED_GAME_EXE_4063000");
		File.WriteAllBytes(Path.Combine(game, "Spider-Man.exe"), gameBytes);
		var assetArchive = Path.Combine(game, "asset_archive");
		Directory.CreateDirectory(assetArchive);
		File.WriteAllBytes(Path.Combine(assetArchive, "toc"), Encoding.ASCII.GetBytes("TOC_FIXTURE"));
		var package = Path.Combine(root, "Web_Swing_Controls_MSMR_v0.2.1-beta.script");
		Directory.CreateDirectory(root);
		if (packageFactory == null) CreatePackage(package);
		else packageFactory(package);
		var gameHash = Hash(gameBytes);
		return new Fixture(game, package, gameHash, NewOptions(game, package, gameHash));
	}

	private static ScriptInstallOptions NewOptions(
		string game,
		string package,
		string gameHash,
		Func<bool>? processGuard = null
	) {
		return new ScriptInstallOptions(
			game,
			package,
			ProxyBytes,
			Hash(ProxyBytes),
			gameHash,
			new[] { Hash(StockProxyBytes) }
		) {
			IsProtectedProcessRunning = processGuard ?? (() => false),
			UtcNow = () => new DateTimeOffset(2026, 9, 1, 12, 0, 0, TimeSpan.Zero)
		};
	}

	private static void CreatePackage(
		string path,
		string? extraEntry = null,
		IReadOnlyList<string>? dependencies = null
	) {
		using var stream = new FileStream(path, FileMode.CreateNew, FileAccess.Write, FileShare.None);
		using var archive = new ZipArchive(stream, ZipArchiveMode.Create);
		WriteEntry(archive, "info.json", Encoding.UTF8.GetBytes(
			"{\n" +
			"  \"game\": \"MSMR\",\n" +
			"  \"name\": \"Web Swing Controls\",\n" +
			"  \"author\": \"saltyb\",\n" +
			"  \"version\": \"0.2.1\",\n" +
			"  \"type\": \"script\",\n" +
			"  \"dependencies\": [" + string.Join(",", (dependencies ?? Array.Empty<string>()).Select(value => "\"" + value + "\"")) + "]\n" +
			"}\n"
		));
		WriteEntry(archive, "TrueSwing.dll", ScriptDllBytes);
		if (extraEntry != null) WriteEntry(archive, extraEntry, Encoding.ASCII.GetBytes("ESCAPE"));
	}

	private static void WriteEntry(ZipArchive archive, string name, byte[] bytes) {
		var entry = archive.CreateEntry(name, CompressionLevel.Optimal);
		using var stream = entry.Open();
		stream.Write(bytes);
	}

	private static SortedDictionary<string, string> Snapshot(string root) {
		return Directory.GetFiles(root, "*", SearchOption.AllDirectories)
			.ToDictionary(
				path => Path.GetRelativePath(root, path),
				path => Hash(File.ReadAllBytes(path)),
				StringComparer.OrdinalIgnoreCase
			)
			.ToSortedDictionary(StringComparer.OrdinalIgnoreCase);
	}

	private static void AssertSnapshotsEqual(
		SortedDictionary<string, string> expected,
		SortedDictionary<string, string> actual,
		string message
	) {
		AssertEqual(expected.Count, actual.Count, message + " File count");
		foreach (var pair in expected) {
			Assert(actual.TryGetValue(pair.Key, out var hash), message + " Missing " + pair.Key);
			AssertEqual(pair.Value, hash, message + " Hash " + pair.Key);
		}
	}

	private static void AssertThrows<T>(Action action, string messageFragment) where T: Exception {
		try {
			action();
		} catch (T ex) {
			Assert(ex.Message.Contains(messageFragment, StringComparison.OrdinalIgnoreCase),
				$"Expected error containing '{messageFragment}', found '{ex.Message}'.");
			return;
		}
		throw new Exception($"Expected {typeof(T).Name} containing '{messageFragment}'.");
	}

	private static void Run(string name, Action test) {
		test();
		++_passed;
		Console.WriteLine("PASS " + name);
	}

	private static void Assert(bool condition, string message) {
		if (!condition) throw new Exception(message);
	}

	private static void AssertEqual<T>(T expected, T actual, string label) {
		if (!EqualityComparer<T>.Default.Equals(expected, actual)) {
			throw new Exception($"{label}: expected '{expected}', found '{actual}'.");
		}
	}

	private static void AssertBytes(byte[] expected, byte[] actual, string label) {
		Assert(expected.SequenceEqual(actual), label + " bytes differ.");
	}

	private static string Hash(byte[] bytes) => Convert.ToHexString(SHA256.HashData(bytes));

	private sealed record Fixture(string Game, string Package, string GameHash, ScriptInstallOptions Options);
}

internal static class DictionaryExtensions {
	public static SortedDictionary<TKey, TValue> ToSortedDictionary<TKey, TValue>(
		this IDictionary<TKey, TValue> dictionary,
		IComparer<TKey> comparer
	) where TKey: notnull {
		var result = new SortedDictionary<TKey, TValue>(comparer);
		foreach (var pair in dictionary) result.Add(pair.Key, pair.Value);
		return result;
	}
}

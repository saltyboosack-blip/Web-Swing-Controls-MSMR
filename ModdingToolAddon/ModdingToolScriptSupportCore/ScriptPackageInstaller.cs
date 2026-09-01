// Unofficial Modding Tool script-install add-on for Marvel's Spider-Man Remastered.
// Distributed under GPL-3.0-or-later with the upstream Modding Tool source.

using System.Diagnostics;
using System.IO.Compression;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace ModdingToolScriptSupportCore;

public enum InstallActionKind {
	Unchanged,
	Create,
	Replace
}

public sealed record ScriptPackageInfo(
	string Name,
	string Author,
	string Version,
	string Game,
	string Type,
	string MainDll,
	string PackageSha256,
	IReadOnlyList<string> Files,
	IReadOnlyList<string> Dependencies
);

public sealed record InstallAction(
	string RelativePath,
	InstallActionKind Kind,
	string? ExistingSha256,
	string DesiredSha256
);

public sealed record ScriptInstallPlan(
	ScriptPackageInfo Package,
	string GameDirectory,
	string GameExecutableSha256,
	IReadOnlyList<InstallAction> Actions
) {
	public bool HasChanges => Actions.Any(action => action.Kind != InstallActionKind.Unchanged);

	public int CreateCount => Actions.Count(action => action.Kind == InstallActionKind.Create);

	public int ReplaceCount => Actions.Count(action => action.Kind == InstallActionKind.Replace);
}

public sealed record ScriptInstallResult(
	ScriptPackageInfo Package,
	string GameDirectory,
	bool Changed,
	string? BackupDirectory,
	IReadOnlyList<InstallAction> Actions
);

public sealed class ScriptInstallOptions {
	public ScriptInstallOptions(
		string gameDirectory,
		string scriptPackagePath,
		byte[] proxyPayload,
		string expectedProxySha256,
		string expectedGameExecutableSha256,
		IEnumerable<string> replaceableProxySha256s
	) {
		GameDirectory = gameDirectory;
		ScriptPackagePath = scriptPackagePath;
		ProxyPayload = proxyPayload;
		ExpectedProxySha256 = expectedProxySha256;
		ExpectedGameExecutableSha256 = expectedGameExecutableSha256;
		ReplaceableProxySha256s = replaceableProxySha256s.ToArray();
	}

	public string GameDirectory { get; }

	public string ScriptPackagePath { get; }

	public byte[] ProxyPayload { get; }

	public string ExpectedProxySha256 { get; }

	public string ExpectedGameExecutableSha256 { get; }

	public IReadOnlyList<string> ReplaceableProxySha256s { get; }

	public string RequiredGameId { get; init; } = "MSMR";

	public long MaximumPackageBytes { get; init; } = 128L * 1024L * 1024L;

	public int MaximumEntries { get; init; } = 1024;

	public Func<bool>? IsProtectedProcessRunning { get; init; }

	public Func<DateTimeOffset>? UtcNow { get; init; }
}

public sealed class ScriptInstallException: Exception {
	public ScriptInstallException(string message): base(message) {}

	public ScriptInstallException(string message, Exception innerException): base(message, innerException) {}
}

public sealed class ScriptPackageInstaller {
	private const string GameExecutableName = "Spider-Man.exe";
	private const string TocName = "toc";
	private const string TocBackupName = "toc.BAK";
	private const string ScriptsDirectoryName = "scripts";
	private const string ScriptsListName = "scripts.txt";
	private const string ProxyName = "winmm.dll";
	private const string CommandLineName = "commandline.txt";
	private const string BackupsDirectoryName = "ModdingTool Script Backups";
	private const int MaximumTextFileBytes = 1024 * 1024;

	private sealed record ParsedPackage(ScriptPackageInfo Info, IReadOnlyDictionary<string, byte[]> Files);

	private sealed record DesiredFile(string RelativePath, string Destination, byte[] Content, InstallAction Action);

	private sealed record CommitRecord(DesiredFile Desired, bool Existed, string? BackupPath);

	public ScriptInstallPlan Inspect(ScriptInstallOptions options) {
		ArgumentNullException.ThrowIfNull(options);
		ValidateHash(options.ExpectedProxySha256, nameof(options.ExpectedProxySha256));
		ValidateHash(options.ExpectedGameExecutableSha256, nameof(options.ExpectedGameExecutableSha256));
		foreach (var hash in options.ReplaceableProxySha256s) {
			ValidateHash(hash, nameof(options.ReplaceableProxySha256s));
		}

		var gameDirectory = ResolveGameDirectory(options.GameDirectory);
		ValidateGameLayout(gameDirectory);
		var gameExecutable = Path.Combine(gameDirectory, GameExecutableName);
		var gameExecutableHash = Sha256File(gameExecutable);
		if (!HashEquals(gameExecutableHash, options.ExpectedGameExecutableSha256)) {
			throw new ScriptInstallException(
				$"Unsupported Spider-Man.exe build. Expected SHA-256 {NormalizeHash(options.ExpectedGameExecutableSha256)}; found {gameExecutableHash}. No files were changed."
			);
		}

		var proxyHash = Sha256Bytes(options.ProxyPayload);
		if (!HashEquals(proxyHash, options.ExpectedProxySha256)) {
			throw new ScriptInstallException(
				$"Embedded script proxy failed verification. Expected SHA-256 {NormalizeHash(options.ExpectedProxySha256)}; found {proxyHash}."
			);
		}

		var package = ReadPackage(options);
		var desiredFiles = BuildDesiredFiles(options, gameDirectory, package, validateProxyTarget: true);
		return new ScriptInstallPlan(
			package.Info,
			gameDirectory,
			gameExecutableHash,
			desiredFiles.Select(file => file.Action).ToArray()
		);
	}

	public ScriptInstallResult Install(ScriptInstallOptions options) {
		ArgumentNullException.ThrowIfNull(options);
		var initialPlan = Inspect(options);
		if (!initialPlan.HasChanges) {
			return new ScriptInstallResult(initialPlan.Package, initialPlan.GameDirectory, false, null, initialPlan.Actions);
		}

		if (IsProtectedProcessRunning(options)) {
			throw new ScriptInstallException(
				"Marvel's Spider-Man Remastered or its video helper is running. Close the game yourself, then try again. The tool will never close it automatically."
			);
		}

		using var mutex = new Mutex(false, GetMutexName(initialPlan.GameDirectory));
		var acquired = false;
		try {
			try {
				acquired = mutex.WaitOne(TimeSpan.Zero);
			} catch (AbandonedMutexException) {
				acquired = true;
			}
			if (!acquired) {
				throw new ScriptInstallException("Another script installation is already running for this game folder.");
			}

			if (IsProtectedProcessRunning(options)) {
				throw new ScriptInstallException(
					"Marvel's Spider-Man Remastered or its video helper started during preflight. Close it yourself, then try again. No files were changed."
				);
			}

			var plan = Inspect(options);
			if (!plan.HasChanges) {
				return new ScriptInstallResult(plan.Package, plan.GameDirectory, false, null, plan.Actions);
			}

			var package = ReadPackage(options);
			if (!HashEquals(package.Info.PackageSha256, plan.Package.PackageSha256)) {
				throw new ScriptInstallException("The selected .script package changed during installation. No files were changed.");
			}

			var desiredFiles = BuildDesiredFiles(options, plan.GameDirectory, package, validateProxyTarget: true)
				.Where(file => file.Action.Kind != InstallActionKind.Unchanged)
				.ToArray();
			if (desiredFiles.Length == 0) {
				return new ScriptInstallResult(plan.Package, plan.GameDirectory, false, null, plan.Actions);
			}

			var backupDirectory = CreateBackupDirectory(options, plan);
			var commitRecords = PrepareBackups(plan.GameDirectory, backupDirectory, desiredFiles);
			WriteManifest(backupDirectory, plan, commitRecords, "prepared", null);

			var committed = new List<CommitRecord>();
			try {
				if (IsProtectedProcessRunning(options)) {
					throw new ScriptInstallException(
						"Marvel's Spider-Man Remastered or its video helper started before the file commit. Close it yourself, then try again. No game files were changed."
					);
				}
				foreach (var record in commitRecords) {
					committed.Add(record);
					WriteAtomically(record.Desired.Destination, record.Desired.Content);
					var installedHash = Sha256File(record.Desired.Destination);
					if (!HashEquals(installedHash, record.Desired.Action.DesiredSha256)) {
						throw new ScriptInstallException(
							$"Post-install verification failed for '{record.Desired.RelativePath}'. Expected {record.Desired.Action.DesiredSha256}; found {installedHash}."
						);
					}
				}

				WriteManifest(backupDirectory, plan, commitRecords, "complete", null);
			} catch (Exception installError) {
				var rollbackErrors = RollBack(committed);
				var rollbackSummary = rollbackErrors.Count == 0
					? "All changed targets were restored."
					: "Rollback had errors: " + string.Join(" | ", rollbackErrors);
				try {
					WriteManifest(backupDirectory, plan, commitRecords, "rolled-back", installError.Message + " " + rollbackSummary);
				} catch {}
				throw new ScriptInstallException(
					$"Script installation failed. {rollbackSummary} Backup preserved at '{backupDirectory}'.",
					installError
				);
			}

			return new ScriptInstallResult(plan.Package, plan.GameDirectory, true, backupDirectory, plan.Actions);
		} finally {
			if (acquired) {
				try { mutex.ReleaseMutex(); } catch {}
			}
		}
	}

	private static ParsedPackage ReadPackage(ScriptInstallOptions options) {
		var packagePath = Path.GetFullPath(options.ScriptPackagePath);
		if (!File.Exists(packagePath)) {
			throw new ScriptInstallException($"Script package not found: {packagePath}");
		}
		if (!Path.GetExtension(packagePath).Equals(".script", StringComparison.OrdinalIgnoreCase)) {
			throw new ScriptInstallException("Select an Overstrike-compatible .script package.");
		}
		AssertRegularFile(packagePath, "Script package");

		var packageLength = new FileInfo(packagePath).Length;
		if (packageLength <= 0 || packageLength > options.MaximumPackageBytes) {
			throw new ScriptInstallException($"Script package size is invalid or exceeds {options.MaximumPackageBytes} bytes.");
		}

		var packageHash = Sha256File(packagePath);
		var files = new Dictionary<string, byte[]>(StringComparer.OrdinalIgnoreCase);
		var orderedNames = new List<string>();
		JsonDocument? infoDocument = null;
		long totalUncompressedBytes = 0;

		try {
			using var stream = new FileStream(packagePath, FileMode.Open, FileAccess.Read, FileShare.Read);
			using var archive = new ZipArchive(stream, ZipArchiveMode.Read, leaveOpen: false);
			if (archive.Entries.Count == 0 || archive.Entries.Count > options.MaximumEntries) {
				throw new ScriptInstallException($"Script package must contain between 1 and {options.MaximumEntries} entries.");
			}

			foreach (var entry in archive.Entries) {
				if (IsSymbolicLink(entry)) {
					throw new ScriptInstallException($"Symbolic links are not allowed in .script packages: {entry.FullName}");
				}
				if (IsDirectoryEntry(entry)) continue;

				var relativePath = NormalizeArchivePath(entry.FullName);
				totalUncompressedBytes = checked(totalUncompressedBytes + entry.Length);
				if (entry.Length < 0 || entry.Length > options.MaximumPackageBytes || totalUncompressedBytes > options.MaximumPackageBytes) {
					throw new ScriptInstallException($"Expanded script package exceeds {options.MaximumPackageBytes} bytes.");
				}

				using var entryStream = entry.Open();
				using var memory = new MemoryStream(entry.Length > int.MaxValue ? 0 : (int)entry.Length);
				entryStream.CopyTo(memory);
				var bytes = memory.ToArray();
				if (bytes.LongLength != entry.Length) {
					throw new ScriptInstallException($"Archive entry length mismatch: {relativePath}");
				}

				if (relativePath.Equals("info.json", StringComparison.OrdinalIgnoreCase)) {
					if (infoDocument != null) {
						throw new ScriptInstallException("The .script package contains more than one info.json.");
					}
					infoDocument = JsonDocument.Parse(bytes);
					continue;
				}

				if (!files.TryAdd(relativePath, bytes)) {
					throw new ScriptInstallException($"Duplicate archive path: {relativePath}");
				}
				orderedNames.Add(relativePath);
			}
		} catch (ScriptInstallException) {
			throw;
		} catch (Exception ex) {
			throw new ScriptInstallException("The selected file is not a valid, readable .script package.", ex);
		}

		using (infoDocument) {
			if (infoDocument == null) {
				throw new ScriptInstallException("The .script package has no root info.json.");
			}
			var root = infoDocument.RootElement;
			var name = RequiredString(root, "name");
			var author = OptionalString(root, "author");
			var version = RequiredString(root, "version");
			var game = RequiredString(root, "game");
			var type = RequiredString(root, "type");
			if (!game.Equals(options.RequiredGameId, StringComparison.OrdinalIgnoreCase)) {
				throw new ScriptInstallException($"This package targets '{game}', not '{options.RequiredGameId}'.");
			}
			if (!type.Equals("script", StringComparison.OrdinalIgnoreCase) && !type.Equals("lib", StringComparison.OrdinalIgnoreCase)) {
				throw new ScriptInstallException($"Unsupported script package type: {type}");
			}

			var dependencies = ReadStringArray(root, "dependencies");
			if (dependencies.Count > 0) {
				throw new ScriptInstallException(
					"This add-on installs dependency-free .script packages only. Required dependencies: " + string.Join(", ", dependencies)
				);
			}

			var mainDll = orderedNames.FirstOrDefault(path =>
				!path.Contains('/') && path.EndsWith(".dll", StringComparison.OrdinalIgnoreCase));
			if (mainDll == null) {
				throw new ScriptInstallException("The .script package has no root-level DLL to load.");
			}

			var info = new ScriptPackageInfo(
				name,
				author,
				version,
				game,
				type,
				mainDll,
				packageHash,
				orderedNames.AsReadOnly(),
				dependencies.AsReadOnly()
			);
			return new ParsedPackage(info, files);
		}
	}

	private static IReadOnlyList<DesiredFile> BuildDesiredFiles(
		ScriptInstallOptions options,
		string gameDirectory,
		ParsedPackage package,
		bool validateProxyTarget
	) {
		var scriptsDirectory = Path.Combine(gameDirectory, ScriptsDirectoryName);
		AssertSafeDirectoryIfPresent(scriptsDirectory, "Game scripts directory");

		var desiredFiles = new List<DesiredFile>();
		foreach (var pair in package.Files) {
			var destination = ResolveChildPath(scriptsDirectory, pair.Key);
			AssertSafeParentChain(gameDirectory, destination, pair.Key);
			var relativeToGame = Path.GetRelativePath(gameDirectory, destination);
			desiredFiles.Add(CreateDesiredFile(relativeToGame, destination, pair.Value));
		}

		var scriptsListPath = Path.Combine(gameDirectory, ScriptsListName);
		var scriptsListText = ReadSmallTextFile(scriptsListPath, ScriptsListName);
		var updatedScriptsList = AddScriptToList(scriptsListText, package.Info.MainDll);
		desiredFiles.Add(CreateDesiredFile(ScriptsListName, scriptsListPath, Utf8(updatedScriptsList)));

		var proxyPath = Path.Combine(gameDirectory, ProxyName);
		if (validateProxyTarget && File.Exists(proxyPath)) {
			AssertRegularFile(proxyPath, "Existing game-directory winmm.dll");
			var existingHash = Sha256File(proxyPath);
			var isFixed = HashEquals(existingHash, options.ExpectedProxySha256);
			var isReplaceable = options.ReplaceableProxySha256s.Any(hash => HashEquals(existingHash, hash));
			if (!isFixed && !isReplaceable) {
				throw new ScriptInstallException(
					$"The game directory already contains an unknown winmm.dll (SHA-256 {existingHash}). Refusing to overwrite another loader. No files were changed."
				);
			}
		}
		desiredFiles.Add(CreateDesiredFile(ProxyName, proxyPath, options.ProxyPayload));

		var commandLinePath = Path.Combine(gameDirectory, CommandLineName);
		var commandLineText = ReadSmallTextFile(commandLinePath, CommandLineName);
		var updatedCommandLine = AddScriptsArgument(commandLineText);
		desiredFiles.Add(CreateDesiredFile(CommandLineName, commandLinePath, Utf8(updatedCommandLine)));

		return desiredFiles;
	}

	private static DesiredFile CreateDesiredFile(string relativePath, string destination, byte[] content) {
		var desiredHash = Sha256Bytes(content);
		string? existingHash = null;
		var kind = InstallActionKind.Create;
		if (File.Exists(destination)) {
			AssertRegularFile(destination, $"Existing target '{relativePath}'");
			existingHash = Sha256File(destination);
			kind = HashEquals(existingHash, desiredHash) ? InstallActionKind.Unchanged : InstallActionKind.Replace;
		} else if (Directory.Exists(destination)) {
			throw new ScriptInstallException($"Install target is a directory, not a file: {destination}");
		}

		var action = new InstallAction(relativePath, kind, existingHash, desiredHash);
		return new DesiredFile(relativePath, destination, content, action);
	}

	private static List<CommitRecord> PrepareBackups(
		string gameDirectory,
		string backupDirectory,
		IReadOnlyList<DesiredFile> desiredFiles
	) {
		var records = new List<CommitRecord>();
		foreach (var desired in desiredFiles) {
			var existed = File.Exists(desired.Destination);
			string? backupPath = null;
			if (existed) {
				AssertRegularFile(desired.Destination, $"Existing target '{desired.RelativePath}'");
				backupPath = ResolveChildPath(Path.Combine(backupDirectory, "originals"), desired.RelativePath);
				var parent = Path.GetDirectoryName(backupPath)
					?? throw new ScriptInstallException($"Could not resolve backup directory for {desired.RelativePath}.");
				Directory.CreateDirectory(parent);
				File.Copy(desired.Destination, backupPath, overwrite: false);
				var backupHash = Sha256File(backupPath);
				if (!HashEquals(backupHash, desired.Action.ExistingSha256!)) {
					throw new ScriptInstallException($"Backup verification failed for '{desired.RelativePath}'.");
				}
			}
			records.Add(new CommitRecord(desired, existed, backupPath));
		}
		return records;
	}

	private static List<string> RollBack(IReadOnlyList<CommitRecord> committed) {
		var errors = new List<string>();
		for (var index = committed.Count - 1; index >= 0; --index) {
			var record = committed[index];
			try {
				if (record.Existed) {
					if (record.BackupPath == null || !File.Exists(record.BackupPath)) {
						throw new IOException("Verified backup is missing.");
					}
					WriteAtomically(record.Desired.Destination, File.ReadAllBytes(record.BackupPath));
				} else if (File.Exists(record.Desired.Destination)) {
					var currentHash = Sha256File(record.Desired.Destination);
					if (!HashEquals(currentHash, record.Desired.Action.DesiredSha256)) {
						throw new IOException("New target changed externally; it was preserved instead of deleted.");
					}
					File.Delete(record.Desired.Destination);
				}
			} catch (Exception ex) {
				errors.Add($"{record.Desired.RelativePath}: {ex.Message}");
			}
		}
		return errors;
	}

	private static string CreateBackupDirectory(ScriptInstallOptions options, ScriptInstallPlan plan) {
		var now = options.UtcNow?.Invoke() ?? DateTimeOffset.UtcNow;
		var safeName = SanitizeFileName(plan.Package.Name);
		var suffix = Guid.NewGuid().ToString("N")[..8];
		var name = $"{now:yyyyMMdd-HHmmssfff}-{safeName}-{suffix}";
		var root = Path.Combine(plan.GameDirectory, BackupsDirectoryName);
		AssertSafeDirectoryIfPresent(root, "Script backup directory");
		var path = Path.Combine(root, name);
		Directory.CreateDirectory(path);
		return path;
	}

	private static void WriteManifest(
		string backupDirectory,
		ScriptInstallPlan plan,
		IReadOnlyList<CommitRecord> records,
		string status,
		string? error
	) {
		var data = new {
			schemaVersion = 1,
			status,
			error,
			package = plan.Package,
			gameDirectory = plan.GameDirectory,
			gameExecutableSha256 = plan.GameExecutableSha256,
			files = records.Select(record => new {
				record.Desired.RelativePath,
				record.Existed,
				originalSha256 = record.Desired.Action.ExistingSha256,
				installedSha256 = record.Desired.Action.DesiredSha256,
				backupRelativePath = record.BackupPath == null
					? null
					: Path.GetRelativePath(backupDirectory, record.BackupPath)
			}).ToArray()
		};
		var json = JsonSerializer.Serialize(data, new JsonSerializerOptions { WriteIndented = true });
		WriteAtomically(Path.Combine(backupDirectory, "install-manifest.json"), Utf8(json + Environment.NewLine));
	}

	private static void WriteAtomically(string destination, byte[] content) {
		var parent = Path.GetDirectoryName(destination)
			?? throw new ScriptInstallException($"Could not resolve destination directory: {destination}");
		Directory.CreateDirectory(parent);
		var temporary = Path.Combine(parent, ".moddingtool-script-" + Guid.NewGuid().ToString("N") + ".tmp");
		try {
			using (var stream = new FileStream(temporary, FileMode.CreateNew, FileAccess.Write, FileShare.None)) {
				stream.Write(content);
				stream.Flush(flushToDisk: true);
			}
			if (!HashEquals(Sha256File(temporary), Sha256Bytes(content))) {
				throw new IOException("Staged file hash mismatch.");
			}

			if (File.Exists(destination)) {
				AssertRegularFile(destination, "Atomic replacement target");
				File.Move(temporary, destination, overwrite: true);
			} else {
				File.Move(temporary, destination);
			}
		} finally {
			if (File.Exists(temporary)) {
				try { File.Delete(temporary); } catch {}
			}
		}
	}

	private static string ResolveGameDirectory(string gameDirectory) {
		if (string.IsNullOrWhiteSpace(gameDirectory)) {
			throw new ScriptInstallException("No game directory is selected. Load the game's toc first.");
		}
		var fullPath = Path.GetFullPath(gameDirectory).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
		if (!Directory.Exists(fullPath)) {
			throw new ScriptInstallException($"Game directory does not exist: {fullPath}");
		}
		var root = Path.GetPathRoot(fullPath)?.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
		if (string.Equals(fullPath, root, StringComparison.OrdinalIgnoreCase)) {
			throw new ScriptInstallException("A drive root is not a valid game directory.");
		}
		return fullPath;
	}

	private static void ValidateGameLayout(string gameDirectory) {
		var executable = Path.Combine(gameDirectory, GameExecutableName);
		if (!File.Exists(executable)) {
			throw new ScriptInstallException($"{GameExecutableName} was not found beside the loaded toc.");
		}
		AssertRegularFile(executable, GameExecutableName);
		var archiveDirectory = Path.Combine(gameDirectory, "asset_archive");
		var hasRootToc = File.Exists(Path.Combine(gameDirectory, TocName))
			|| File.Exists(Path.Combine(gameDirectory, TocBackupName));
		var hasArchiveToc = File.Exists(Path.Combine(archiveDirectory, TocName))
			|| File.Exists(Path.Combine(archiveDirectory, TocBackupName));
		if (!hasRootToc && !hasArchiveToc) {
			throw new ScriptInstallException("The selected game folder has neither toc nor toc.BAK in its root or asset_archive folder.");
		}
	}

	private static string ResolveChildPath(string root, string relativePath) {
		var fullRoot = Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
		var fullPath = Path.GetFullPath(Path.Combine(fullRoot, relativePath.Replace('/', Path.DirectorySeparatorChar)));
		var prefix = fullRoot + Path.DirectorySeparatorChar;
		if (!fullPath.StartsWith(prefix, StringComparison.OrdinalIgnoreCase)) {
			throw new ScriptInstallException($"Path escapes its allowed directory: {relativePath}");
		}
		return fullPath;
	}

	private static string NormalizeArchivePath(string path) {
		if (string.IsNullOrWhiteSpace(path)) {
			throw new ScriptInstallException("The .script package contains an empty path.");
		}
		var normalized = path.Replace('\\', '/');
		if (normalized.StartsWith('/') || normalized.Contains(':')) {
			throw new ScriptInstallException($"Absolute or drive-qualified archive path is not allowed: {path}");
		}
		var segments = normalized.Split('/');
		foreach (var segment in segments) {
			if (segment.Length == 0 || segment == "." || segment == "..") {
				throw new ScriptInstallException($"Unsafe archive path: {path}");
			}
			if (segment.EndsWith(' ') || segment.EndsWith('.') || segment.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0) {
				throw new ScriptInstallException($"Archive path is not a safe Windows filename: {path}");
			}
			var baseName = Path.GetFileNameWithoutExtension(segment);
			if (IsReservedWindowsName(baseName)) {
				throw new ScriptInstallException($"Archive path uses a reserved Windows name: {path}");
			}
		}
		return string.Join('/', segments);
	}

	private static string AddScriptToList(string existing, string mainDll) {
		var lines = existing.Split(new[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries);
		if (lines.Any(line => line.Trim().Equals(mainDll, StringComparison.OrdinalIgnoreCase))) {
			return existing;
		}
		if (string.IsNullOrWhiteSpace(existing)) return mainDll;
		var separator = existing.EndsWith('\r') || existing.EndsWith('\n') ? "" : Environment.NewLine;
		return existing + separator + mainDll;
	}

	private static string AddScriptsArgument(string existing) {
		var tokens = existing.Split((char[]?)null, StringSplitOptions.RemoveEmptyEntries);
		if (tokens.Any(token => token.Equals("-scripts", StringComparison.OrdinalIgnoreCase))) {
			return existing;
		}
		if (existing.Length == 0) return "-scripts";
		var separator = char.IsWhiteSpace(existing[^1]) ? "" : " ";
		return existing + separator + "-scripts";
	}

	private static string ReadSmallTextFile(string path, string label) {
		if (!File.Exists(path)) return "";
		AssertRegularFile(path, label);
		var length = new FileInfo(path).Length;
		if (length > MaximumTextFileBytes) {
			throw new ScriptInstallException($"{label} is unexpectedly large; refusing to rewrite it.");
		}
		return File.ReadAllText(path);
	}

	private static string RequiredString(JsonElement root, string propertyName) {
		if (!root.TryGetProperty(propertyName, out var value) || value.ValueKind != JsonValueKind.String) {
			throw new ScriptInstallException($"info.json is missing string property '{propertyName}'.");
		}
		var text = value.GetString()?.Trim() ?? "";
		if (text.Length == 0) {
			throw new ScriptInstallException($"info.json property '{propertyName}' is empty.");
		}
		return text;
	}

	private static string OptionalString(JsonElement root, string propertyName) {
		if (!root.TryGetProperty(propertyName, out var value) || value.ValueKind != JsonValueKind.String) return "";
		return value.GetString()?.Trim() ?? "";
	}

	private static List<string> ReadStringArray(JsonElement root, string propertyName) {
		var result = new List<string>();
		if (!root.TryGetProperty(propertyName, out var value)) return result;
		if (value.ValueKind != JsonValueKind.Array) {
			throw new ScriptInstallException($"info.json property '{propertyName}' must be an array.");
		}
		foreach (var item in value.EnumerateArray()) {
			if (item.ValueKind != JsonValueKind.String || string.IsNullOrWhiteSpace(item.GetString())) {
				throw new ScriptInstallException($"info.json property '{propertyName}' contains an invalid entry.");
			}
			result.Add(item.GetString()!.Trim());
		}
		return result;
	}

	private static bool IsProtectedProcessRunning(ScriptInstallOptions options) {
		if (options.IsProtectedProcessRunning != null) return options.IsProtectedProcessRunning();
		return IsProcessRunning("Spider-Man") || IsProcessRunning("crs-video");
	}

	private static bool IsProcessRunning(string processName) {
		Process[] processes;
		try {
			processes = Process.GetProcessesByName(processName);
		} catch {
			return true;
		}
		try {
			return processes.Length > 0;
		} finally {
			foreach (var process in processes) process.Dispose();
		}
	}

	private static string GetMutexName(string gameDirectory) {
		var pathHash = Sha256Bytes(Encoding.UTF8.GetBytes(gameDirectory.ToUpperInvariant()));
		return "Local\\MSMRModdingToolScriptInstaller-" + pathHash[..24];
	}

	private static string SanitizeFileName(string value) {
		var invalid = Path.GetInvalidFileNameChars();
		var builder = new StringBuilder();
		foreach (var character in value) {
			builder.Append(invalid.Contains(character) ? '_' : character);
		}
		var result = builder.ToString().Trim().TrimEnd('.', ' ');
		if (result.Length == 0 || IsReservedWindowsName(Path.GetFileNameWithoutExtension(result))) result = "script";
		return result.Length > 64 ? result[..64] : result;
	}

	private static bool IsReservedWindowsName(string value) {
		return value.Equals("CON", StringComparison.OrdinalIgnoreCase)
			|| value.Equals("PRN", StringComparison.OrdinalIgnoreCase)
			|| value.Equals("AUX", StringComparison.OrdinalIgnoreCase)
			|| value.Equals("NUL", StringComparison.OrdinalIgnoreCase)
			|| (value.Length == 4 && value.StartsWith("COM", StringComparison.OrdinalIgnoreCase) && value[3] is >= '1' and <= '9')
			|| (value.Length == 4 && value.StartsWith("LPT", StringComparison.OrdinalIgnoreCase) && value[3] is >= '1' and <= '9');
	}

	private static bool IsDirectoryEntry(ZipArchiveEntry entry) {
		return entry.Name.Length == 0 && (entry.FullName.EndsWith('/') || entry.FullName.EndsWith('\\'));
	}

	private static bool IsSymbolicLink(ZipArchiveEntry entry) {
		const int UnixFileTypeMask = 0xF000;
		const int UnixSymbolicLink = 0xA000;
		var unixMode = (entry.ExternalAttributes >> 16) & UnixFileTypeMask;
		return unixMode == UnixSymbolicLink;
	}

	private static void AssertRegularFile(string path, string label) {
		if (!File.Exists(path)) throw new ScriptInstallException($"{label} is missing: {path}");
		var attributes = File.GetAttributes(path);
		if ((attributes & FileAttributes.ReparsePoint) != 0) {
			throw new ScriptInstallException($"{label} must be a regular file, not a symbolic link or reparse point: {path}");
		}
	}

	private static void AssertSafeDirectoryIfPresent(string path, string label) {
		if (!Directory.Exists(path)) return;
		var attributes = File.GetAttributes(path);
		if ((attributes & FileAttributes.ReparsePoint) != 0) {
			throw new ScriptInstallException($"{label} must not be a symbolic link or reparse point: {path}");
		}
	}

	private static void AssertSafeParentChain(string allowedRoot, string destination, string label) {
		var fullRoot = Path.GetFullPath(allowedRoot).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
		var parent = Path.GetDirectoryName(Path.GetFullPath(destination))
			?? throw new ScriptInstallException($"Could not resolve parent path for {label}.");
		var relativeParent = Path.GetRelativePath(fullRoot, parent);
		if (relativeParent == ".") return;

		var current = fullRoot;
		foreach (var segment in relativeParent.Split(
			new[] { Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar },
			StringSplitOptions.RemoveEmptyEntries
		)) {
			current = Path.Combine(current, segment);
			if (File.Exists(current)) {
				throw new ScriptInstallException($"A file blocks the destination directory for {label}: {current}");
			}
			if (!Directory.Exists(current)) continue;
			var attributes = File.GetAttributes(current);
			if ((attributes & FileAttributes.ReparsePoint) != 0) {
				throw new ScriptInstallException($"A symbolic link or reparse point blocks safe installation of {label}: {current}");
			}
		}
	}

	private static void ValidateHash(string hash, string label) {
		if (hash == null || hash.Length != 64 || !hash.All(Uri.IsHexDigit)) {
			throw new ScriptInstallException($"{label} is not a valid SHA-256 value.");
		}
	}

	private static string NormalizeHash(string hash) => hash.ToUpperInvariant();

	private static bool HashEquals(string left, string right) {
		return left.Equals(right, StringComparison.OrdinalIgnoreCase);
	}

	private static string Sha256File(string path) {
		using var stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.Read);
		return Convert.ToHexString(SHA256.HashData(stream));
	}

	private static string Sha256Bytes(byte[] bytes) {
		return Convert.ToHexString(SHA256.HashData(bytes));
	}

	private static byte[] Utf8(string text) => new UTF8Encoding(encoderShouldEmitUTF8Identifier: false).GetBytes(text);
}

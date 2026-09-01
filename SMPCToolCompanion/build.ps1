[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ControlsDllPath,

    [Parameter(Mandatory = $true)]
    [string]$ProxyDllPath,

    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$expectedControlsHash = '10F25A79F541731BAF898F28316B4FBE444E96E07C6B47F46E49C55F9AB691FC'
$expectedProxyHash = 'FFB6AD902798D7DF87F6D3FDE3B5A4BAE1E9212A40971B346452A5064768E111'

$controls = (Resolve-Path -LiteralPath $ControlsDllPath).Path
$proxy = (Resolve-Path -LiteralPath $ProxyDllPath).Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $output) {
    throw "Refusing existing output directory: $output"
}

$controlsHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $controls).Hash.ToUpperInvariant()
$proxyHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $proxy).Hash.ToUpperInvariant()
if ($controlsHash -ne $expectedControlsHash) {
    throw "Controls DLL hash mismatch. Expected $expectedControlsHash; found $controlsHash."
}
if ($proxyHash -ne $expectedProxyHash) {
    throw "Proxy DLL hash mismatch. Expected $expectedProxyHash; found $proxyHash."
}

$compiler = Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
if (-not (Test-Path -LiteralPath $compiler)) {
    throw "Microsoft .NET Framework compiler not found: $compiler"
}

$sourceDirectory = Join-Path $PSScriptRoot 'src'
$testDirectory = Join-Path $PSScriptRoot 'tests'
$testBin = Join-Path $output 'test-bin'
$testArtifacts = Join-Path $output 'test-artifacts'
$release = Join-Path $output 'Web_Swing_Controls_SMPCTool_Companion_v1.1.0-rc2'
$payload = Join-Path $release 'payload'
[void](New-Item -ItemType Directory -Path $testBin)
[void](New-Item -ItemType Directory -Path $release)
[void](New-Item -ItemType Directory -Path $payload)

$testExecutable = Join-Path $testBin 'InstallerCoreTests.exe'
$testCompileArguments = @(
    '/nologo',
    '/target:exe',
    '/platform:x64',
    '/optimize+',
    '/debug-',
    "/out:$testExecutable",
    '/reference:System.dll',
    '/reference:System.Core.dll',
    (Join-Path $sourceDirectory 'InstallerCore.cs'),
    (Join-Path $testDirectory 'InstallerCoreTests.cs')
)
& $compiler @testCompileArguments
if ($LASTEXITCODE -ne 0) {
    throw "Test compilation failed with exit code $LASTEXITCODE."
}

& $testExecutable $testArtifacts
if ($LASTEXITCODE -ne 0) {
    throw "Installer tests failed with exit code $LASTEXITCODE."
}

$application = Join-Path $release 'WebSwingControls_SMPCTool_Companion.exe'
$applicationCompileArguments = @(
    '/nologo',
    '/target:winexe',
    '/platform:x64',
    '/optimize+',
    '/debug-',
    "/out:$application",
    '/reference:System.dll',
    '/reference:System.Core.dll',
    '/reference:System.Drawing.dll',
    '/reference:System.Windows.Forms.dll',
    (Join-Path $sourceDirectory 'AssemblyInfo.cs'),
    (Join-Path $sourceDirectory 'InstallerCore.cs'),
    (Join-Path $sourceDirectory 'Program.cs')
)
& $compiler @applicationCompileArguments
if ($LASTEXITCODE -ne 0) {
    throw "Companion compilation failed with exit code $LASTEXITCODE."
}

Copy-Item -LiteralPath $controls -Destination (Join-Path $payload 'TrueSwing.dll')
Copy-Item -LiteralPath $proxy -Destination (Join-Path $payload 'winmm.dll')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'README_RELEASE.txt') -Destination (Join-Path $release 'README_FIRST.txt')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot '..\LICENSE') -Destination (Join-Path $release 'LICENSE.txt')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'THIRD_PARTY_NOTICES.md') -Destination $release

$releaseFiles = Get-ChildItem -LiteralPath $release -Recurse -File | Sort-Object FullName
$hashLines = foreach ($file in $releaseFiles) {
    $relative = $file.FullName.Substring($release.Length + 1).Replace('\', '/')
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $file.FullName).Hash.ToUpperInvariant()
    "$hash  $relative"
}
$hashLines | Set-Content -LiteralPath (Join-Path $release 'SHA256SUMS.txt') -Encoding ASCII

$applicationHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $application).Hash.ToUpperInvariant()
Write-Host 'SMPCTOOL_COMPANION_BUILD_OK'
Write-Host "Application SHA-256: $applicationHash"
Write-Host "Release: $release"
Write-Host "Tests: $testArtifacts"

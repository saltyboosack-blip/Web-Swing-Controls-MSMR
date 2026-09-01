[CmdletBinding()]
param(
    [string]$ProxyPath = (Join-Path $PSScriptRoot '..\ScriptsProxy\x64\Release\scripts_proxy.dll'),
    [string]$OutputDirectory = (Join-Path $PSScriptRoot 'build-addon\publish')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$expectedProxyHash = 'FFB6AD902798D7DF87F6D3FDE3B5A4BAE1E9212A40971B346452A5064768E111'
$resolvedProxy = (Resolve-Path -LiteralPath $ProxyPath).Path
$actualProxyHash = (Get-FileHash -LiteralPath $resolvedProxy -Algorithm SHA256).Hash.ToUpperInvariant()
if ($actualProxyHash -ne $expectedProxyHash) {
    throw "Proxy hash mismatch. Expected $expectedProxyHash; found $actualProxyHash."
}

$payloadDirectory = Join-Path $PSScriptRoot 'ModdingTool\NativePayload'
[void](New-Item -ItemType Directory -Path $payloadDirectory -Force)
Copy-Item -LiteralPath $resolvedProxy -Destination (Join-Path $payloadDirectory 'scripts_proxy.dll') -Force

& dotnet run --project (Join-Path $PSScriptRoot 'ModdingToolScriptSupportTests\ModdingToolScriptSupportTests.csproj') `
    -c Release -- (Join-Path $PSScriptRoot 'build-addon-tests')
if ($LASTEXITCODE -ne 0) { throw "Script installer tests failed with exit code $LASTEXITCODE." }

& dotnet publish (Join-Path $PSScriptRoot 'ModdingTool\ModdingTool.csproj') `
    -c Release -r win-x64 --self-contained false `
    -p:PublishSingleFile=true -p:PublishReadyToRun=false `
    -o $OutputDirectory
if ($LASTEXITCODE -ne 0) { throw "Modding Tool publish failed with exit code $LASTEXITCODE." }

$toolPath = Join-Path $OutputDirectory 'ModdingTool.exe'
$toolHash = (Get-FileHash -LiteralPath $toolPath -Algorithm SHA256).Hash.ToUpperInvariant()
Write-Host "ADDON_BUILD_OK"
Write-Host "ModdingTool.exe SHA-256: $toolHash"
Write-Host "Output: $OutputDirectory"


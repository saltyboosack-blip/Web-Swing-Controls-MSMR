param(
    [string]$BinaryPath = (Join-Path $PSScriptRoot 'x64\Release\winmm.dll'),
    [string]$DefinitionPath = (Join-Path $PSScriptRoot 'winmm.def'),
    [string]$AssemblyPath = (Join-Path $PSScriptRoot 'winmm_forwarders.asm'),
    [string]$BaselinePath
)

$ErrorActionPreference = 'Stop'

function Assert-True {
    param([bool]$Condition, [string]$Message)
    if (-not $Condition) {
        throw $Message
    }
}

function Find-Dumpbin {
    $root = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC'
    $candidate = Get-ChildItem -LiteralPath $root -Filter dumpbin.exe -Recurse |
        Where-Object { $_.FullName -match 'Hostx64\\x64' } |
        Sort-Object FullName -Descending |
        Select-Object -First 1
    if ($null -eq $candidate) {
        throw 'Could not locate x64 dumpbin.exe.'
    }
    return $candidate.FullName
}

function Read-DefinitionEntries {
    param([string]$Path)
    $result = foreach ($line in Get-Content -LiteralPath $Path) {
        if ($line -match '^\s*([^=\s]+)=([^\s]+)\s+@(\d+)\s*$') {
            [pscustomobject]@{
                ExportName = $Matches[1]
                ThunkName = $Matches[2]
                Ordinal = [int]$Matches[3]
            }
        }
    }
    return @($result)
}

function Read-BinaryExports {
    param([string]$Path, [string]$Dumpbin)
    $output = & $Dumpbin /nologo /exports $Path
    if ($LASTEXITCODE -ne 0) {
        throw "dumpbin /exports failed for $Path."
    }
    $result = foreach ($line in $output) {
        if ($line -match '^\s*(\d+)\s+[0-9A-F]+\s+([0-9A-F]{8})\s+(\S+)(?:\s+=\s+(\S+))?\s*$') {
            [pscustomobject]@{
                Ordinal = [int]$Matches[1]
                Rva = [Convert]::ToUInt32($Matches[2], 16)
                ExportName = $Matches[3]
                ThunkName = $Matches[4]
            }
        }
    }
    return @($result | Sort-Object Ordinal)
}

function Assert-ExportParity {
    param(
        [object[]]$Expected,
        [object[]]$Actual,
        [string]$Label,
        [bool]$CheckThunkNames
    )
    Assert-True ($Actual.Count -eq $Expected.Count) `
        "$Label export count mismatch: expected $($Expected.Count), got $($Actual.Count)."
    for ($index = 0; $index -lt $Expected.Count; ++$index) {
        $wanted = $Expected[$index]
        $found = $Actual[$index]
        Assert-True ($found.Ordinal -eq $wanted.Ordinal) `
            "$Label ordinal mismatch at index $index."
        Assert-True ($found.ExportName -ceq $wanted.ExportName) `
            "$Label name mismatch at ordinal $($wanted.Ordinal)."
        if ($CheckThunkNames) {
            Assert-True ($found.ThunkName -ceq $wanted.ThunkName) `
                "$Label thunk mismatch at ordinal $($wanted.Ordinal)."
        }
    }
}

function Get-PeLayout {
    param([byte[]]$Bytes)
    $peOffset = [BitConverter]::ToInt32($Bytes, 0x3c)
    Assert-True ($Bytes[$peOffset] -eq 0x50 -and
        $Bytes[$peOffset + 1] -eq 0x45 -and
        $Bytes[$peOffset + 2] -eq 0 -and
        $Bytes[$peOffset + 3] -eq 0) 'Invalid PE signature.'
    $machine = [BitConverter]::ToUInt16($Bytes, $peOffset + 4)
    Assert-True ($machine -eq 0x8664) 'Proxy is not x64.'
    $sectionCount = [BitConverter]::ToUInt16($Bytes, $peOffset + 6)
    $optionalSize = [BitConverter]::ToUInt16($Bytes, $peOffset + 20)
    $optionalOffset = $peOffset + 24
    Assert-True ([BitConverter]::ToUInt16($Bytes, $optionalOffset) -eq 0x20b) `
        'Proxy is not PE32+.'
    $dllCharacteristics = [BitConverter]::ToUInt16($Bytes, $optionalOffset + 70)
    Assert-True (($dllCharacteristics -band 0x40) -ne 0) 'ASLR is not enabled.'
    Assert-True (($dllCharacteristics -band 0x100) -ne 0) 'NX is not enabled.'

    $sections = @()
    $sectionOffset = $optionalOffset + $optionalSize
    for ($index = 0; $index -lt $sectionCount; ++$index) {
        $offset = $sectionOffset + ($index * 40)
        $nameBytes = $Bytes[$offset..($offset + 7)]
        $name = [Text.Encoding]::ASCII.GetString($nameBytes).TrimEnd([char]0)
        $sections += [pscustomobject]@{
            Name = $name
            VirtualSize = [BitConverter]::ToUInt32($Bytes, $offset + 8)
            VirtualAddress = [BitConverter]::ToUInt32($Bytes, $offset + 12)
            RawSize = [BitConverter]::ToUInt32($Bytes, $offset + 16)
            RawOffset = [BitConverter]::ToUInt32($Bytes, $offset + 20)
            Characteristics = [BitConverter]::ToUInt32($Bytes, $offset + 36)
        }
    }
    return [pscustomobject]@{ Sections = $sections }
}

function Find-PeSection {
    param([object[]]$Sections, [uint32]$Rva)
    foreach ($section in $Sections) {
        $span = [Math]::Max([uint64]$section.VirtualSize, [uint64]$section.RawSize)
        if ([uint64]$Rva -ge [uint64]$section.VirtualAddress -and
            [uint64]$Rva -lt ([uint64]$section.VirtualAddress + $span)) {
            return $section
        }
    }
    throw ('RVA 0x{0:X8} is outside all sections.' -f $Rva)
}

function Convert-RvaToFileOffset {
    param([object[]]$Sections, [uint32]$Rva)
    $section = Find-PeSection $Sections $Rva
    return [int]([uint64]$section.RawOffset +
        ([uint64]$Rva - [uint64]$section.VirtualAddress))
}

$definition = Read-DefinitionEntries $DefinitionPath
Assert-True ($definition.Count -eq 180) `
    "Expected 180 definition entries; got $($definition.Count)."
for ($index = 0; $index -lt $definition.Count; ++$index) {
    Assert-True ($definition[$index].Ordinal -eq ($index + 1)) `
        "Definitions are not contiguous at index $index."
}
Assert-True (($definition.ExportName | Sort-Object -Unique).Count -eq 180) `
    'Definition contains duplicate export names.'
Assert-True (($definition.ThunkName | Sort-Object -Unique).Count -eq 180) `
    'Definition contains duplicate thunk names.'

$dumpbin = Find-Dumpbin
$binary = (Resolve-Path -LiteralPath $BinaryPath).Path
$exports = Read-BinaryExports $binary $dumpbin
Assert-ExportParity $definition $exports 'Fixed proxy' $true

if ($BaselinePath) {
    $baseline = (Resolve-Path -LiteralPath $BaselinePath).Path
    $baselineExports = Read-BinaryExports $baseline $dumpbin
    Assert-ExportParity $definition $baselineExports 'Baseline proxy' $false
}

$assembly = Get-Content -LiteralPath $AssemblyPath -Raw
Assert-True ($assembly -notmatch '(?m)^\s*(PA|runASM)\b') `
    'Shared forwarding state remains in generated assembly.'
for ($index = 0; $index -lt $definition.Count; ++$index) {
    $thunk = [Regex]::Escape($definition[$index].ThunkName)
    $offset = $index * 8
    $pattern = "(?m)^PUBLIC $thunk\r?\n$thunk PROC\r?\n[ \t]+jmp QWORD PTR \[g_winmmFunctions \+ $offset\]\r?\n$thunk ENDP\r?$"
    Assert-True ([Regex]::IsMatch($assembly, $pattern)) `
        "Generated thunk mismatch for $($definition[$index].ThunkName)."
}

[byte[]]$bytes = [IO.File]::ReadAllBytes($binary)
$layout = Get-PeLayout $bytes
$firstSlotRva = $null
$seenRvas = [Collections.Generic.HashSet[uint32]]::new()
for ($index = 0; $index -lt $exports.Count; ++$index) {
    [uint32]$rva = $exports[$index].Rva
    Assert-True ($seenRvas.Add($rva)) `
        "Duplicate thunk RVA at ordinal $($exports[$index].Ordinal)."
    $section = Find-PeSection $layout.Sections $rva
    Assert-True (($section.Characteristics -band 0x20000000) -ne 0) `
        "Thunk is not executable at ordinal $($exports[$index].Ordinal)."
    Assert-True (($section.Characteristics -band 0x80000000) -eq 0) `
        "Thunk section is writable at ordinal $($exports[$index].Ordinal)."
    $fileOffset = Convert-RvaToFileOffset $layout.Sections $rva
    Assert-True ($bytes[$fileOffset] -eq 0xff -and $bytes[$fileOffset + 1] -eq 0x25) `
        "Thunk is not a direct RIP-relative jump at ordinal $($exports[$index].Ordinal)."
    $displacement = [BitConverter]::ToInt32($bytes, $fileOffset + 2)
    [uint32]$slotRva = [uint32]([int64]$rva + 6 + [int64]$displacement)
    if ($null -eq $firstSlotRva) {
        $firstSlotRva = $slotRva
    }
    [uint32]$expectedSlotRva = [uint32]([uint64]$firstSlotRva + ($index * 8))
    Assert-True ($slotRva -eq $expectedSlotRva) `
        "Thunk slot mismatch at ordinal $($exports[$index].Ordinal)."
    $slotSection = Find-PeSection $layout.Sections $slotRva
    Assert-True (($slotSection.Characteristics -band 0x80000000) -ne 0) `
        "Target slot is not writable data at ordinal $($exports[$index].Ordinal)."
    Assert-True (($slotSection.Characteristics -band 0x20000000) -eq 0) `
        "Target slot is executable at ordinal $($exports[$index].Ordinal)."
}

$headers = (& $dumpbin /nologo /headers $binary) -join "`n"
Assert-True ($headers -match '8664 machine \(x64\)') 'dumpbin did not report x64.'
Assert-True ($headers -match 'Dynamic base') 'dumpbin did not report dynamic base.'
Assert-True ($headers -match 'NX compatible') 'dumpbin did not report NX compatibility.'
Assert-True ($headers -match '(?m)^\s+\.reloc name\s*$') 'Relocation section is absent.'

$dependents = (& $dumpbin /nologo /dependents $binary) -join "`n"
Assert-True ($dependents -match '(?im)^\s+KERNEL32\.dll\s*$') `
    'KERNEL32 dependency is absent.'
Assert-True ($dependents -match '(?im)^\s+USER32\.dll\s*$') `
    'USER32 dependency is absent.'
Assert-True ($dependents -notmatch '(?i)VCRUNTIME|MSVCP\d|ucrtbase') `
    'Dynamic C/C++ runtime dependency found.'

$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $binary).Hash
$baselineResult = if ($BaselinePath) { 'matched' } else { 'not-requested' }
Write-Host "PROXY_VERIFY_OK exports=180 thunks=180 baseline=$baselineResult sha256=$hash"

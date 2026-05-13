param(
    [Parameter(Mandatory = $true)]
    [string]$TargetDir,
    [Parameter(Mandatory = $false)]
    [string]$SourceRoot = ""
)

$ErrorActionPreference = "Stop"

function Resolve-CandidatePath {
    param([string]$PathValue)
    if ([string]::IsNullOrWhiteSpace($PathValue)) { return $null }
    try {
        return (Resolve-Path -LiteralPath $PathValue -ErrorAction Stop).Path
    } catch {
        return $null
    }
}

if (-not (Test-Path -LiteralPath $TargetDir)) {
    New-Item -ItemType Directory -Path $TargetDir -Force | Out-Null
}

$dllNames = @("libssl-1_1-x64.dll", "libcrypto-1_1-x64.dll", "Qt5Xml.dll")

$candidateDirs = @()
if (-not [string]::IsNullOrWhiteSpace($env:OPENSSL_DLL_DIR)) {
    $candidateDirs += $env:OPENSSL_DLL_DIR
}

if (-not [string]::IsNullOrWhiteSpace($SourceRoot)) {
    $candidateDirs += @(
        (Join-Path $SourceRoot "build-noip-proxy-fallback\release"),
        (Join-Path $SourceRoot "build-proxy-tools-check\release"),
        (Join-Path $SourceRoot "build-noip-msvc150-release\release"),
        (Join-Path $SourceRoot "build-noip-msvc-release\release")
    )
}

$qtRoot = "C:\Qt\Tools"
if (Test-Path -LiteralPath $qtRoot) {
    $candidateDirs += Get-ChildItem -Path $qtRoot -Directory -ErrorAction SilentlyContinue |
        ForEach-Object { Join-Path $_.FullName "opt\bin" }
}

if (-not [string]::IsNullOrWhiteSpace($env:QTDIR)) {
    $candidateDirs += (Join-Path $env:QTDIR "bin")
}

$candidateDirs += @(
    "C:\Qt\5.15.0\msvc2019_64\bin",
    "C:\Qt\5.15.2\msvc2019\bin"
)

$resolvedDirs = $candidateDirs |
    ForEach-Object { Resolve-CandidatePath $_ } |
    Where-Object { $_ -and (Test-Path -LiteralPath $_) } |
    Select-Object -Unique

foreach ($dllName in $dllNames) {
    $targetFile = Join-Path $TargetDir $dllName
    if (Test-Path -LiteralPath $targetFile) {
        continue
    }

    $sourceFile = $null
    foreach ($dir in $resolvedDirs) {
        $candidate = Join-Path $dir $dllName
        if (Test-Path -LiteralPath $candidate) {
            $sourceFile = $candidate
            break
        }
    }

    if ($null -eq $sourceFile) {
        Write-Host "[copy-openssl] Missing $dllName (no source found)."
        continue
    }

    Copy-Item -LiteralPath $sourceFile -Destination $targetFile -Force
    Write-Host "[copy-openssl] Copied $dllName from $sourceFile"
}

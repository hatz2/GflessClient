<#
.SYNOPSIS
    Packages PowCaptchaSolver into a self-contained .exe (no console window).
    The launcher expects it as PowCaptchaSolver.exe next to GflessClient.exe.
#>

param(
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = $PSScriptRoot
$DistDir     = Join-Path $ProjectRoot 'dist'
$ExeName     = 'PowCaptchaSolver'

function Write-Step($msg) {
    Write-Host "`n>>> $msg" -ForegroundColor Cyan
}

function Write-OK($msg) {
    Write-Host "[OK] $msg" -ForegroundColor Green
}

function Fail($msg) {
    Write-Host "[ERROR] $msg" -ForegroundColor Red
    exit 1
}

Write-Step "Checking prerequisites..."
$python = Get-Command 'python' -ErrorAction SilentlyContinue
if (-not $python) { Fail "Python not found. Install from https://python.org" }
Write-OK "$($python.Source)"

if ($Clean) {
    Write-Step "Cleaning previous builds..."
    Remove-Item -Recurse -Force $DistDir, (Join-Path $ProjectRoot 'build') -ErrorAction SilentlyContinue
    Remove-Item "$ProjectRoot\*.spec" -ErrorAction SilentlyContinue
    Write-OK "Clean done"
}

Write-Step "Running the tests..."
Set-Location $ProjectRoot
& $python -m unittest discover -s tests
if ($LASTEXITCODE -ne 0) { Fail "tests failed" }
Write-OK "Tests passed"

Write-Step "Installing PyInstaller..."
& $python -m pip install pyinstaller
if ($LASTEXITCODE -ne 0) { Fail "pip install pyinstaller failed" }

Write-Step "Packaging with PyInstaller..."
$scriptPath = Join-Path $ProjectRoot 'src\pow_captcha_solver.py'
& $python -m PyInstaller --onefile --noconsole --name $ExeName $scriptPath --distpath $DistDir --clean --noconfirm
if ($LASTEXITCODE -ne 0) { Fail "PyInstaller failed" }

$exePath = Join-Path $DistDir "$ExeName.exe"
if (Test-Path $exePath) {
    $size = (Get-Item $exePath).Length
    Write-Host "`n==================================" -ForegroundColor Cyan
    Write-Host "  BUILD COMPLETED" -ForegroundColor Green
    Write-Host "==================================" -ForegroundColor Cyan
    Write-Host "Output : $exePath"
    Write-Host "Size   : $([math]::Round($size / 1MB, 2)) MB"
    Write-Host "Copy it next to GflessClient.exe (the installer does this for you)."
}

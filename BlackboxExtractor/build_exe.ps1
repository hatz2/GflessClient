<#
.SYNOPSIS
    Builds the Frida agent and packages BlackboxExtractor into a self-contained .exe.
#>

param(
    [switch]$Clean,
    [switch]$UseVenb
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = $PSScriptRoot
$DistDir     = Join-Path $ProjectRoot 'dist'
$ExeName     = 'BlackboxExtractor'
$AgentDir    = Join-Path $ProjectRoot 'build'

function Write-Step($msg) {
    Write-Host "`n>>> $msg" -ForegroundColor Cyan
}

function Write-OK($msg) {
    Write-Host "[OK] $msg" -ForegroundColor Green
}

function Write-Error($msg) {
    Write-Host "[ERROR] $msg" -ForegroundColor Red
    exit 1
}

# ──────────────────────────────────────────────
# 1. Check prerequisites
# ──────────────────────────────────────────────
Write-Step "Checking prerequisites..."

$node = Get-Command 'node' -ErrorAction SilentlyContinue
if (-not $node) { Write-Error "Node.js not found. Install from https://nodejs.org" }
Write-OK "Node.js $($node.Version)"

$python = Get-Command 'python' -ErrorAction SilentlyContinue
if (-not $python) { Write-Error "Python not found. Install from https://python.org" }
Write-OK "$($python.Source)"

# ──────────────────────────────────────────────
# 2. Optional clean
# ──────────────────────────────────────────────
if ($Clean) {
    Write-Step "Cleaning previous builds..."
    Remove-Item -Recurse -Force $DistDir, $AgentDir, "$ProjectRoot\__pycache__" -ErrorAction SilentlyContinue
    Remove-Item "$ProjectRoot\*.spec" -ErrorAction SilentlyContinue
    Write-OK "Clean done"
}

# ──────────────────────────────────────────────
# 3. Optional virtual env
# ──────────────────────────────────────────────
$pip = 'pip'
if ($UseVenb) {
    $venvPath = Join-Path $ProjectRoot '.venb'
    if (-not (Test-Path $venvPath)) {
        Write-Step "Creating virtual env..."
        & $python -m venv $venvPath
    }
    $activate = Join-Path $venvPath 'Scripts\Activate.ps1'
    . $activate
    $pip = (Get-Command 'pip' -ErrorAction SilentlyContinue).Source
    Write-OK "Virtual env activated: $venvPath"
}

# ──────────────────────────────────────────────
# 4. Build Frida agent
# ──────────────────────────────────────────────
Write-Step "Building Frida agent..."
Set-Location $ProjectRoot

Write-Host "[*] npm install..."
& 'npm' install
if ($LASTEXITCODE -ne 0) { Write-Error "npm install failed" }

Write-Host "[*] npm run build..."
& 'npm' run build
if ($LASTEXITCODE -ne 0) { Write-Error "npm run build failed" }

if (-not (Test-Path (Join-Path $AgentDir 'agent.js'))) {
    Write-Error "build/agent.js not found"
}
Write-OK "Frida agent built"

# ──────────────────────────────────────────────
# 5. Install PyInstaller
# ──────────────────────────────────────────────
Write-Step "Installing PyInstaller..."
& $pip install pyinstaller
if ($LASTEXITCODE -ne 0) { Write-Error "pip install pyinstaller failed" }
Write-OK "PyInstaller installed"

# ──────────────────────────────────────────────
# 6. Package with PyInstaller
# ──────────────────────────────────────────────
Write-Step "Packaging with PyInstaller..."
$scriptPath = Join-Path $ProjectRoot 'src\client.py'
$agentData = "build\agent.js;build"

& pyinstaller --onefile --add-data $agentData --name $ExeName $scriptPath --distpath $DistDir --clean --noconfirm
if ($LASTEXITCODE -ne 0) { Write-Error "PyInstaller failed" }

# ──────────────────────────────────────────────
# 7. Show result
# ──────────────────────────────────────────────
$exePath = Join-Path $DistDir "$ExeName.exe"
if (Test-Path $exePath) {
    $size = (Get-Item $exePath).Length
    Write-Host "`n==================================" -ForegroundColor Cyan
    Write-Host "  BUILD COMPLETED" -ForegroundColor Green
    Write-Host "==================================" -ForegroundColor Cyan
    Write-Host "Output : $exePath"
    Write-Host "Size   : $([math]::Round($size / 1MB, 2)) MB"
}

Set-Location $ProjectRoot

# YTfy Build Script for Windows (MinGW-w64)
# This script configures and builds the project, then moves the binary and build residues to 'compilation-builds'

$ProjectDir = Get-Location
$BuildDir = Join-Path $ProjectDir "build"
$OutputDir = Join-Path $ProjectDir "compilation-builds"

# Ensure MinGW is available in this session
$MinGWBin = "C:\msys64\mingw64\bin"
if (Test-Path $MinGWBin) {
    $env:Path = "$MinGWBin;" + $env:Path
}
else {
    Write-Host "WARNING: MinGW not found at $MinGWBin. Make sure it is installed." -ForegroundColor Red
}

# Verify cmake and gcc are available
$cmakePath = Get-Command cmake -ErrorAction SilentlyContinue
$gccPath = Get-Command gcc   -ErrorAction SilentlyContinue
if (-not $cmakePath) { Write-Host "ERROR: cmake not found in PATH." -ForegroundColor Red; exit 1 }
if (-not $gccPath) { Write-Host "ERROR: gcc not found in PATH."   -ForegroundColor Red; exit 1 }
Write-Host "cmake : $($cmakePath.Source)" -ForegroundColor DarkGray
Write-Host "gcc   : $($gccPath.Source)"   -ForegroundColor DarkGray

# Ensure output directory exists
if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
    Write-Host "Created output directory: $OutputDir" -ForegroundColor Cyan
}

# Create or clear build directory
if (Test-Path $BuildDir) {
    Remove-Item -Path $BuildDir -Recurse -Force
}
New-Item -ItemType Directory -Path $BuildDir | Out-Null

# Navigate to build directory
Push-Location $BuildDir

Write-Host "--- Configuring Project with CMake (Ninja + MinGW-w64) ---" -ForegroundColor Yellow
cmake .. `
    -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_C_COMPILER="$MinGWBin\gcc.exe" `
    -DCMAKE_CXX_COMPILER="$MinGWBin\g++.exe" `
    -DCMAKE_MAKE_PROGRAM="$MinGWBin\ninja.exe" `
    -DCMAKE_PREFIX_PATH="C:/msys64/mingw64"

Write-Host "--- Building Project ---" -ForegroundColor Yellow
cmake --build .

# Check if build was successful
if ($LASTEXITCODE -eq 0) {
    Write-Host "Build Successful!" -ForegroundColor Green
    
    # Move binary to output folder (Release subfolder is common in MSVC)
    $ExePath = Get-ChildItem -Path . -Filter "YTfy.exe" -Recurse | Select-Object -First 1
    if ($ExePath) {
        Copy-Item -Path $ExePath.FullName -Destination $OutputDir
        Write-Host "Binary copied to: $(Join-Path $OutputDir YTfy.exe)" -ForegroundColor Green
    }

    # Copy build residues (the entire build folder content) to output/residues
    $ResidueDir = Join-Path $OutputDir "residues"
    if (Test-Path $ResidueDir) { Remove-Item -Path $ResidueDir -Recurse -Force }
    New-Item -ItemType Directory -Path $ResidueDir
    
    Copy-Item -Path "*.*" -Destination $ResidueDir -Recurse
    Write-Host "Build residues moved to: $ResidueDir" -ForegroundColor Cyan
}
else {
    Write-Host "Build Failed with exit code $LASTEXITCODE" -ForegroundColor Red
}

Pop-Location
Write-Host "Done." -ForegroundColor Cyan

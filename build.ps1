# Build Script en PowerShell para La Cueva MultiStream
Write-Host "========================================================" -ForegroundColor Cyan
Write-Host "   Compilador Asistido - La Cueva MultiStream Plugin" -ForegroundColor Yellow
Write-Host "========================================================" -ForegroundColor Cyan
Write-Host ""

# 1. Verificar CMake
$cmakePath = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmakePath) {
    Write-Host "[ERROR] CMake no esta instalado o no se encuentra en el PATH." -ForegroundColor Red
    Write-Host "Puedes descargarlo desde: https://cmake.org/download/" -ForegroundColor Yellow
    Write-Host "O instalarlo con: winget install Kitware.CMake" -ForegroundColor Green
    exit 1
} else {
    Write-Host "[OK] CMake detectado: $($cmakePath.Source)" -ForegroundColor Green
}

# 2. Verificar Visual Studio 2022
$vsPaths = @(
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
)

$foundVs = $null
foreach ($path in $vsPaths) {
    if (Test-Path $path) {
        $foundVs = $path
        break
    }
}

if (-not $foundVs) {
    Write-Host "[ADVERTENCIA] No se encontro Visual Studio 2022 en las rutas estandar." -ForegroundColor Yellow
    Write-Host "Se requiere la carga de trabajo 'Desarrollo para el escritorio con C++'." -ForegroundColor Yellow
} else {
    Write-Host "[OK] Visual Studio 2022 detectado en: $foundVs" -ForegroundColor Green
}

# 3. Verificar OBS Studio
$obsPath = "C:\Program Files\obs-studio"
if (Test-Path $obsPath) {
    Write-Host "[OK] OBS Studio detectado en: $obsPath" -ForegroundColor Green
} else {
    Write-Host "[INFO] OBS Studio no encontrado en Program Files. Si esta en otra ruta, especificala." -ForegroundColor Gray
}

# 4. Configurar con CMake
Write-Host ""
Write-Host "[1/2] Configurando proyecto con CMake..." -ForegroundColor Cyan

if (Test-Path "build") {
    Remove-Item -Path "build" -Recurse -Force
}

cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "[ERROR] La configuracion de CMake fallo." -ForegroundColor Red
    Write-Host "Esto ocurre usualmente cuando falta Qt6 o los headers de libobs en el sistema." -ForegroundColor Yellow
    Write-Host "Recuerda que tambien puedes usar el instalador precompilado oficial de obs-multi-rtmp." -ForegroundColor Cyan
    exit 1
}

# 5. Compilar
Write-Host ""
Write-Host "[2/2] Compilando DLL en modo Release..." -ForegroundColor Cyan
cmake --build build --config Release

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "========================================================" -ForegroundColor Green
    Write-Host "   EXITO: Plugin compilado correctamente en:" -ForegroundColor Green
    Write-Host "   build\Release\lacueva-multistream.dll" -ForegroundColor Yellow
    Write-Host "========================================================" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "[ERROR] Fallo la compilacion." -ForegroundColor Red
}

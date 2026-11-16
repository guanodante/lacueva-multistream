# Script para subir el código a tu cuenta de GitHub
Clear-Host
Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "   SUBIR TU PLUGIN 'LA CUEVA MULTISTREAM' A GITHUB" -ForegroundColor Yellow
Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host ""

# Verificar Git
$gitCmd = Get-Command git -ErrorAction SilentlyContinue
if (-not $gitCmd) {
    Write-Host "[ERROR] Git no esta instalado en tu PC." -ForegroundColor Red
    Write-Host "Puedes descargarlo rapido desde: https://git-scm.com/download/win" -ForegroundColor Yellow
    Write-Host "O instalarlo abriendo otra terminal con: winget install Git.Git" -ForegroundColor Green
    Read-Host "Presiona Enter para salir..."
    exit 1
}

Write-Host "Paso 1: Ve a GitHub en tu navegador (la pestaña que tienes abierta)." -ForegroundColor Green
Write-Host "Paso 2: Inicia sesion y dale al boton verde 'New' o '+' para crear un repositorio." -ForegroundColor Green
Write-Host "Paso 3: Ponle de nombre: lacueva-multistream (dejalo en Publico)." -ForegroundColor Green
Write-Host "Paso 4: Copia el link de tu nuevo repositorio (ejemplo: https://github.com/TuUsuario/lacueva-multistream.git)" -ForegroundColor Green
Write-Host ""

$repoUrl = Read-Host "Pega aqui el link de tu repositorio de GitHub y presiona Enter"
$repoUrl = $repoUrl.Trim()

if ([string]::IsNullOrWhiteSpace($repoUrl)) {
    Write-Host "[ERROR] No ingresaste ninguna direccion." -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "[1/4] Inicializando repositorio Git local..." -ForegroundColor Cyan
git init
git branch -M main

Write-Host "[2/4] Agregando todos los archivos de La Cueva del Dolor..." -ForegroundColor Cyan
git add .

Write-Host "[3/4] Creando version 1.0.0..." -ForegroundColor Cyan
git commit -m "Version inicial 1.0.0 - La Cueva MultiStream con soporte Aitum Vertical"

Write-Host "[4/4] Conectando y subiendo a tu GitHub..." -ForegroundColor Cyan
git remote remove origin 2>$null
git remote add origin $repoUrl
git push -u origin main --force

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "==========================================================" -ForegroundColor Green
    Write-Host "   ¡LISTO! Tu codigo ya esta en tu cuenta de GitHub." -ForegroundColor Green
    Write-Host "   Los servidores de GitHub estan compilando tu .exe ahora." -ForegroundColor Yellow
    Write-Host "==========================================================" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "[AVISO] Si te pide usuario y contrasenia en el navegador, autoriza a Git para terminar de subir." -ForegroundColor Yellow
}

Read-Host "Presiona Enter para cerrar esta ventana..."

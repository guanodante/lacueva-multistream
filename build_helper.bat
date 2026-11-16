@echo off
echo ========================================================
echo   Compilador Asistido - La Cueva MultiStream Plugin
echo ========================================================
echo.

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1"
pause

<#
    Compila PSPTorqueOBD con pspdev en Docker.

    Uso:
        powershell -ExecutionPolicy Bypass -File tools\build.ps1
        powershell -ExecutionPolicy Bypass -File tools\build.ps1 -Clean

    Requiere Docker Desktop en marcha y la imagen pspdev/pspdev.
#>

param(
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$root = $root -replace '\\', '/'

$command = 'export PATH=/usr/local/pspdev/bin:$PATH; make'
if ($Clean) {
    $command = 'export PATH=/usr/local/pspdev/bin:$PATH; make clean; make'
}

Write-Host "Proyecto: $root" -ForegroundColor Cyan
Write-Host "Imagen  : pspdev/pspdev" -ForegroundColor Cyan

$args = @(
    "run", "--rm",
    "-v", "${root}:/src",
    "-w", "/src",
    "pspdev/pspdev",
    "bash", "-lc", $command
)

& docker @args
if ($LASTEXITCODE -ne 0) {
    throw "La compilación ha fallado (código $LASTEXITCODE)."
}

$dist = Join-Path $root "dist\PSPTorqueOBD"
New-Item -ItemType Directory -Force -Path $dist | Out-Null
Copy-Item (Join-Path $root "EBOOT.PBP") $dist -Force
Copy-Item (Join-Path $root "host.txt") $dist -Force
Copy-Item (Join-Path $root "RUN.txt") $dist -Force

Write-Host ""
Write-Host "EBOOT.PBP generado en $root" -ForegroundColor Green
Write-Host "Carpeta lista para copiar: $dist" -ForegroundColor Green
Write-Host "Destino en la PSP: ms0:/PSP/GAME/PSPTorqueOBD/" -ForegroundColor Green

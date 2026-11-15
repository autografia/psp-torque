# Compila y ejecuta las pruebas del nucleo portable en Linux (Docker).
# Uso: powershell -ExecutionPolicy Bypass -File tools\test_core.ps1 [-Rebuild]

param([switch]$Rebuild)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$image = "psp-obd-tests"

$imageId = docker images -q $image
if (-not $imageId -or $Rebuild) {
    Write-Host "Construyendo imagen de pruebas ($image)..."
    docker build -t $image -f (Join-Path $PSScriptRoot "test.Dockerfile") $PSScriptRoot
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$mount = $root -replace '\\', '/'
docker run --rm -v "${mount}:/src" -w /src $image sh -lc "make -C tests clean all"
exit $LASTEXITCODE

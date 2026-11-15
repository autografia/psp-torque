# Copia un log de la PSP al proyecto y muestra un resumen util.
# Uso:
#   powershell -ExecutionPolicy Bypass -File tools\read_log.ps1
#   powershell -ExecutionPolicy Bypass -File tools\read_log.ps1 -Log phase0.log
#   powershell -ExecutionPolicy Bypass -File tools\read_log.ps1 -Drive F:

param(
    [string]$Log = "phase1.log",
    [string]$Drive = "E:"
)

$src = Join-Path $Drive "PSP\GAME\PSPTorqueOBD\$Log"
$dst = Join-Path $PSScriptRoot "..\$Log"

if (-not (Test-Path $src)) {
    Write-Host "No existe $src"
    Write-Host "Conecta la PSP en modo USB, o ejecuta antes la app en la PSP."
    exit 1
}

Copy-Item $src $dst -Force
Write-Host "Copiado a $dst"
Write-Host ""
Write-Host "--- primeras lineas ---"
Get-Content $dst -TotalCount 25

Write-Host ""
Write-Host "--- eventos ---"
Select-String -Path $dst -Pattern 'mascara|ATDPN|ATRV|Adaptador|BARO|RESULTADO|PIDs activos|Peticiones|Enlace|netconf estado' |
    ForEach-Object { $_.Line }

Write-Host ""
Write-Host "--- ultimas muestras ---"
$telemetry = Select-String -Path $dst -Pattern '^t='
if ($telemetry) {
    $telemetry | Select-Object -Last 10 | ForEach-Object { $_.Line }
} else {
    Get-Content $dst -Tail 15
}

Write-Host ""
$text = Get-Content $dst -Raw
if ($text -match "RESULTADO: OK") {
    Write-Host "RESUMEN: correcto" -ForegroundColor Green
} else {
    Write-Host "RESUMEN: incompleto o fallido" -ForegroundColor Yellow
    if ($text -match "Sin IP") { Write-Host " - No se obtuvo IP WiFi" }
    if ($text -match "TCP error") { Write-Host " - No se pudo abrir el socket TCP" }
    if ($text -match "sin adaptador") { Write-Host " - Ningun destino respondio" }
}

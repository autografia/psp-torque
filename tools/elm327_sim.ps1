<#
    ELM327 WiFi simulator — SOLO PARA DESARROLLO
    ------------------------------------------------------------
    Sirve un socket TCP compatible con el protocolo de línea del
    ELM327. Los datos son SINTÉTICOS y no proceden de ningún coche.
    No usar para diagnóstico real.

    Uso:
        powershell -ExecutionPolicy Bypass -File tools\elm327_sim.ps1
        powershell -ExecutionPolicy Bypass -File tools\elm327_sim.ps1 -Port 35000

    La PSP debe apuntar a la IP del PC (host.txt) y estar en la misma red.
#>

param(
    [int]$Port = 35000
)

$ErrorActionPreference = "Stop"

function Write-Response {
    param(
        [System.IO.StreamWriter]$Writer,
        [string]$Text
    )
    # El ELM327 termina las respuestas con CR.
    $Writer.Write($Text + "`r")
    $Writer.Flush()
}

$script:HeadersOn = $false
$script:SpacesOn = $false
$script:CurrentHeader = "7DF"

function Format-Response {
    param(
        [string]$Header,
        [string]$Payload
    )

    $clean = ($Payload -replace '\s+', '')
    if ($script:SpacesOn) {
        $parts = @()
        for ($i = 0; $i -lt $clean.Length; $i += 2) {
            $parts += $clean.Substring($i, 2)
        }
        $clean = $parts -join ' '
    }

    if ($script:HeadersOn) {
        if ($script:SpacesOn) {
            return "$Header $clean"
        }
        return "$Header$clean"
    }
    return $clean
}

function Send-Lines {
    param(
        [System.IO.StreamWriter]$Writer,
        [string[]]$Lines
    )
    foreach ($line in $Lines) {
        Write-Response $Writer $line
    }
}

function Get-SimulatedPid {
    param([string]$Command)

    switch ($Command) {
        "0100" { return "41 00 BE 3F B8 10" }  # máscara simulada
        "0120" { return "41 20 80 00 00 00" }
        "0101" { return "41 01 00 07 E1 00" }
        "0103" { return "41 03 02 00" }
        "0104" { return "41 04 32" }           # carga 19.6 %
        "0105" { return "41 05 78" }           # ECT 80 °C
        "0106" { return "41 06 80" }           # STFT 0 %
        "0107" { return "41 07 80" }           # LTFT 0 %
        "010B" { return "41 0B 23" }           # MAP 35 kPa (ralentí simulado)
        "010C" { return "41 0C 0F A0" }        # 1000 rpm
        "010D" { return "41 0D 32" }           # 50 km/h
        "010E" { return "41 0E 80" }           # avance 0°
        "010F" { return "41 0F 3C" }           # IAT 20 °C
        "0110" { return "41 10 00 64" }        # MAF 1.00 g/s
        "0111" { return "41 11 20" }           # acelerador 12.5 %
        "0114" { return "41 14 80 80" }        # O2 B1S1 0.64 V, trim 0 %
        "012F" { return "41 2F 80" }           # nivel 50 %
        "0133" { return "41 33 65" }           # baro 101 kPa
        "0142" { return "41 42 30 39" }        # 12.345 V
        "0149" { return "41 49 20" }           # pedal D 12.5 %
        "015C" { return "41 5C 5A" }           # aceite 50 °C
        "015E" { return "41 5E 00 14" }        # 1.0 L/h
        "0902" { return "49 02 01 5A 41 52 41 52 45 41 30 30 30 30 30 30 30 30 30" }
        default { return $null }
    }
}

$listener = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Any, $Port)
$listener.Start()

Write-Host ""
Write-Host "ELM327 SIMULATOR — DATOS SINTETICOS, NO REALES" -ForegroundColor Yellow
Write-Host "Escuchando en 0.0.0.0:$Port" -ForegroundColor Yellow
Write-Host "Pulsa Ctrl+C para salir." -ForegroundColor Yellow
Write-Host ""

$clientNumber = 0

while ($true) {
    $client = $listener.AcceptTcpClient()
    $clientNumber++
    $remote = $client.Client.RemoteEndPoint
    Write-Host "[cliente $clientNumber] conectado: $remote" -ForegroundColor Cyan

    $stream = $client.GetStream()
    $reader = [System.IO.StreamReader]::new($stream, [System.Text.Encoding]::ASCII)
    $writer = [System.IO.StreamWriter]::new($stream, [System.Text.Encoding]::ASCII)
    $writer.AutoFlush = $true
    $writer.NewLine = "`r"

    try {
        while ($true) {
            # Lectura manual: el ELM327 termina las ordenes con CR solo.
            # StreamReader.ReadLine() se queda esperando el siguiente byte
            # para comprobar si el salto es CRLF, lo que retrasa la respuesta.
            $line = ""
            $c = -1
            while ($true) {
                $c = $reader.Read()
                if ($c -lt 0) { break }
                if ($c -eq 13) { break }
                if ($c -eq 10) { continue }
                $line += [char]$c
            }
            if ($c -lt 0 -and $line.Length -eq 0) {
                break
            }

            $cmd = $line.Trim().ToUpperInvariant()
            if ($cmd.Length -eq 0) {
                continue
            }

            Write-Host "  > $cmd"

            switch -Regex ($cmd) {
                '^ATZ$' {
                    Start-Sleep -Milliseconds 120
                    Write-Response $writer "ELM327 v1.5 (SIM)"
                    continue
                }
                '^ATI$' {
                    Write-Response $writer "ELM327 v1.5 (SIM)"
                    continue
                }
                '^ATE0$|^ATL0$|^ATSP0$|^ATSP6$|^ATAT1$|^ATAL$|^ATNL$' {
                    Write-Response $writer "OK"
                    continue
                }
                '^ATH0$' {
                    $script:HeadersOn = $false
                    Write-Response $writer "OK"
                    continue
                }
                '^ATH1$' {
                    $script:HeadersOn = $true
                    Write-Response $writer "OK"
                    continue
                }
                '^ATS0$' {
                    $script:SpacesOn = $false
                    Write-Response $writer "OK"
                    continue
                }
                '^ATS1$' {
                    $script:SpacesOn = $true
                    Write-Response $writer "OK"
                    continue
                }
                '^ATSH\s+([0-9A-F]+)$' {
                    $script:CurrentHeader = $Matches[1]
                    Write-Response $writer "OK"
                    continue
                }
                '^ATD$' {
                    $script:HeadersOn = $false
                    $script:SpacesOn = $false
                    $script:CurrentHeader = "7DF"
                    Write-Response $writer "OK"
                    continue
                }
                '^ATDPN$' {
                    Write-Response $writer "A6"
                    continue
                }
                '^ATDP$' {
                    Write-Response $writer "ISO 15765-4 (CAN 11/500) (SIM)"
                    continue
                }
                '^ATRV$' {
                    Write-Response $writer "12.4V"
                    continue
                }
                '^AT@1$' {
                    Write-Response $writer "PSP Torque simulator"
                    continue
                }
                '^0100$' {
                    $a = Format-Response "7E8" "06 41 00 BE 3E B8 11"
                    $b = Format-Response "7E9" "06 41 00 98 3A 80 01"
                    if ($script:HeadersOn) {
                        Send-Lines $writer @($a, $b)
                    }
                    else {
                        Write-Response $writer $a
                    }
                    continue
                }
                '^0120$' {
                    $a = Format-Response "7E8" "06 41 20 80 02 20 01"
                    $b = Format-Response "7E9" "06 41 20 80 00 00 00"
                    if ($script:HeadersOn) {
                        Send-Lines $writer @($a, $b)
                    }
                    else {
                        Write-Response $writer $a
                    }
                    continue
                }
                '^0140$' {
                    $a = Format-Response "7E8" "06 41 40 C0 C0 00 00"
                    if ($script:HeadersOn) {
                        Send-Lines $writer @($a)
                    }
                    else {
                        Write-Response $writer $a
                    }
                    continue
                }
                '^0101$|^0103$|^0104$|^0105$|^0106$|^0107$|^010B$|^010C$|^010D$|^010E$|^010F$|^0110$|^0111$|^0114$|^012F$|^0133$|^0142$|^0149$|^015C$|^015E$|^0902$' {
                    $response = Get-SimulatedPid $cmd
                    if ($null -eq $response) {
                        Write-Response $writer "NO DATA"
                    }
                    else {
                        Write-Response $writer (Format-Response "7E8" $response)
                    }
                    continue
                }
                '^3E00$' {
                    switch ($script:CurrentHeader) {
                        "7E0" { $response = Format-Response "7E8" "02 7E 00" }
                        "7E1" { $response = Format-Response "7E9" "02 7E 00" }
                        "7E2" { $response = Format-Response "7EA" "03 7F 3E 11" }
                        default { $response = "NO DATA" }
                    }
                    Write-Response $writer $response
                    continue
                }
                '^22F1([0-9A-F]{2})$' {
                    $did = $Matches[1]
                    $response = $null
                    $respHeader = $null

                    switch ($script:CurrentHeader) {
                        "7E0" {
                            $respHeader = "7E8"
                            switch ($did) {
                                "90" { $response = "62 F1 90 5A 41 52 41 53 30 30 30 30 30 30 30 30 30 30 30 30" }
                                "97" { $response = "62 F1 97 4D 4F 54 4F 52" }
                                "87" { $response = "62 F1 87 35 35 32 30 30 30 30 31" }
                                "88" { $response = "62 F1 88 31 32 33 34" }
                                "8C" { $response = "62 F1 8C 53 4E 30 30 30 31" }
                            }
                        }
                        "7E1" {
                            $respHeader = "7E9"
                            switch ($did) {
                                "90" { $response = "62 F1 90 5A 41 52 41 53 31 31 31 31 31 31 31 31 31 31 31 31" }
                                "97" { $response = "62 F1 97 43 41 4D 42 49 4F" }
                                "87" { $response = "62 F1 87 35 35 32 30 30 30 30 32" }
                                "88" { $response = "62 F1 88 35 36 37 38" }
                                "8C" { $response = "62 F1 8C 53 4E 30 30 30 32" }
                            }
                        }
                        "7E2" {
                            $respHeader = "7EA"
                            switch ($did) {
                                "90" { $response = "62 F1 90 5A 41 52 41 53 32 32 32 32 32 32 32 32 32 32 32 32" }
                                "97" { $response = "62 F1 97 53 55 53 50 45 4E 53 49 4F 4E" }
                                "87" { $response = "62 F1 87 35 31 38 30 30 30 30 32" }
                                "88" { $response = "62 F1 88 39 30 31 32" }
                                "8C" { $response = "62 F1 8C 53 4E 30 30 30 33" }
                            }
                        }
                    }

                    if ($null -eq $response) {
                        Write-Response $writer "NO DATA"
                    }
                    else {
                        Write-Response $writer (Format-Response $respHeader $response)
                    }
                    continue
                }
                '^AT' {
                    Write-Response $writer "?"
                    continue
                }
                default {
                    Write-Response $writer "NO DATA"
                }
            }
        }
    }
    catch {
        Write-Host "  [cliente desconectado: $($_.Exception.Message)]" -ForegroundColor DarkGray
    }
    finally {
        $reader.Dispose()
        $writer.Dispose()
        $stream.Dispose()
        $client.Close()
        Write-Host "Cliente $clientNumber cerrado" -ForegroundColor DarkGray
    }
}

# Cliente de prueba para tools/elm327_sim.ps1
$client = [System.Net.Sockets.TcpClient]::new("127.0.0.1", 35000)
$stream = $client.GetStream()
$writer = [System.IO.StreamWriter]::new($stream, [System.Text.Encoding]::ASCII)
$writer.AutoFlush = $true
$writer.NewLine = "`r"

$writer.Write("ATI`r")
$writer.Write("0100`r")
$writer.Write("010C`r")
$writer.Write("010B`r")
$writer.Write("0105`r")
$writer.Write("0110`r")

Start-Sleep -Milliseconds 500

$buffer = New-Object byte[] 4096
$read = $stream.Read($buffer, 0, $buffer.Length)
[System.Text.Encoding]::ASCII.GetString($buffer, 0, $read)

$writer.Dispose()
$stream.Dispose()
$client.Close()

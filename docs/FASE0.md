# Fase 0 — Prueba de red y ELM327

> **Estado:** código compilado y `EBOOT.PBP` generado. **Pendiente de ejecutar
> en la PSP real** y de confirmar el modo WiFi del adaptador.

Objetivo: validar el eslabón más arriesgado antes de escribir el dashboard:

```
PSP  ──WiFi──►  ELM327 (o simulador)  ──TCP──►  respuestas AT/PID en crudo
```

## 1. Compilar

Requisitos: Docker Desktop en marcha y la imagen `pspdev/pspdev`.

```bash
MSYS_NO_PATHCONV=1 docker run --rm \
  -v "D:/psp_torqueOBD:/src" -w /src \
  pspdev/pspdev \
  bash -lc 'export PATH=/usr/local/pspdev/bin:$PATH; make'
```

Salida esperada:

```
PSPTorqueOBD.prx
mksfoex ... 'PSP Torque OBD' PARAM.SFO
pack-pbp EBOOT.PBP ...
```

Archivos generados en la raíz:

- `EBOOT.PBP` — lo que se copia a la PSP.
- `PSPTorqueOBD.prx`, `PSPTorqueOBD.elf` — intermedios.

> En esta máquina se ha compilado correctamente con `pspdev` (GCC 15.2,
> PSPSDK de 2026). El Makefile usa `build.mak` (no `build.mk`) y no enlaza
> manualmente `libpspnet_inet`/`libpsputility`: el compilador de pspdev ya las
> añade al final, y así se evita el warning de `psp-fixup-imports`.

## 2. Copiar a la PSP

La forma más cómoda:

```powershell
powershell -ExecutionPolicy Bypass -File tools\build.ps1
```

Además de compilar, crea `dist\PSPTorqueOBD\` con `EBOOT.PBP` y `host.txt`.
Copiar esa carpeta a:

```
ms0:/PSP/GAME/PSPTorqueOBD/
```

con:

```
ms0:/PSP/GAME/PSPTorqueOBD/EBOOT.PBP
ms0:/PSP/GAME/PSPTorqueOBD/host.txt
ms0:/PSP/GAME/PSPTorqueOBD/RUN.txt   (opcional, recordatorio)
```

`host.txt` contiene la IP y el puerto del destino. Por defecto:

```
192.168.0.10 35000
```

Se admiten varias líneas; la app las prueba en orden. Si ninguna responde,
prueba también la puerta de enlace que le haya dado el DHCP:

```
192.168.0.10 35000
192.168.1.5 35000
```

## 3. Configurar la WiFi

1. Encender el ELM327 (en el coche o con alimentación de 12 V).
2. En la PSP: **Ajustes → Ajustes de red → Modo infraestructura**.
3. Crear o editar una conexión para la red del ELM327 (`OBDII`, `V-LINK`, etc.).
4. La red debe ser abierta, WEP o WPA-TKIP para la PSP de fábrica. Con ARK4 y
   `wpa2psp` también puede ser WPA2-AES.
5. Guardar. No hace falta que la prueba de Internet funcione: el adaptador no
   da salida a Internet.

## 4. Ejecutar

1. Lanzar `PSP Torque OBD` desde el menú de juegos.
2. La app abre el diálogo de configuración de red de la PSP. Elegir la conexión
   del ELM327 y conectar.
3. Al volver, la app espera hasta 20 s a obtener IP. Si el diálogo no conectó,
   intenta el perfil guardado número 1 como respaldo.
4. Muestra IP, SSID, seguridad, señal y gateway.
5. Prueba cada destino de `host.txt` (y la puerta de enlace) y se queda con el
   primero que abra TCP y conteste a `ATI`. Después envía:

```
ATE0
ATL0
ATS0
ATH0
ATSP0
ATDPN
ATDP
ATRV
0100
010C
010D
010B
0105
010F
```

6. Se ve la respuesta cruda de cada comando. Con el motor apagado y solo el
   contacto, algunas respuestas serán `NO DATA`; es normal. `ATI` y `ATRV`
   deben responder siempre si el adaptador está alimentado.
7. La app escribe `phase0.log` en la misma carpeta y termina con
   `RESULTADO: OK` o `RESULTADO: FALLO`. Si algo falla, ese archivo contiene
   todo lo que se mostró en pantalla.
8. `START` para salir.

Para leer el log desde el PC:

```powershell
powershell -ExecutionPolicy Bypass -File tools\read_log.ps1 -Log phase0.log
```

## 5. Probar sin coche, con el simulador

El simulador sirve para validar la red y el parser, **no** para diagnosis real.

En el PC:

```powershell
powershell -ExecutionPolicy Bypass -File tools\elm327_sim.ps1
```

- Escucha en `0.0.0.0:35000`.
- Datos sintéticos marcados `(SIM)`.
- Permite un cliente a la vez.

En la PSP:

1. Conectar la PSP a la misma red que el PC (no a la red del ELM327).
2. Averiguar la IP del PC con `ipconfig` (IPv4).
3. Editar `host.txt` con esa IP y puerto `35000`.
4. Abrir el puerto 35000 en el Firewall de Windows si hace falta.
5. Ejecutar la app y elegir la conexión de esa red.

Cliente de prueba local (opcional):

```powershell
powershell -ExecutionPolicy Bypass -File tools\test_client.ps1
```

## 6. Interpretación de errores

| Síntoma | Causa probable | Acción |
|---|---|---|
| `net common: 0x...` con error | Módulo ya cargado o CFW sin módulos de red | Reiniciar la PSP; comprobar que el WiFi funciona en el XMB |
| `Sin IP` | El diálogo no conectó o la red no da DHCP | Configurar IP estática `192.168.0.11/24`, gateway `192.168.0.10` |
| Se queda en el diálogo de red | Se cerró al ver el estado `NONE` inicial | Corregido en el build `fase0 2026-11-05`; usar ese EBOOT |
| `TCP error -3` | Timeout de conexión | IP/puerto incorrectos, firewall, AP sin DHCP |
| `TCP error -4` | Conexión rechazada | ELM327 no escucha en ese puerto o ya hay otro cliente |
| `Ningun destino respondio` | Ningún socket contestó a `ATI` | Añadir la IP correcta en `host.txt`; revisar que solo la PSP esté conectada al ELM327 |
| `El adaptador no responde en ...` | Hubo TCP pero no datos | Otro cliente WiFi conectado al ELM327, o el módulo no está listo |
| `> ATI` sin respuesta | Socket abierto pero adaptador mudo | Probar `ATZ`, revisar alimentación, otro cliente conectado |
| `NO DATA` en todo | Motor apagado / ECU sin contacto / protocolo mal | Poner contacto; probar `ATSP0`; revisar `ATDPN` |
| `?` | Comando AT no reconocido | Clon de ELM327; probar sin `ATS0`/`ATH0` |
| Se queda en `SEARCHING...` | El adaptador busca protocolo | Esperar; con `ATSP6` fijo se evita la búsqueda |

## 7. Criterios de aceptación de la fase 0

- [x] El EBOOT arranca en la PSP-2000/3000 con ARK4.
- [x] La PSP obtiene IP en la red del ELM327 (abierta).
- [x] El socket TCP conecta a `192.168.0.10:35000`.
- [x] `ATI` devuelve la versión del adaptador.
- [x] `ATRV` devuelve una tensión plausible.
- [x] `0100` devuelve una máscara de PID (o `NO DATA` si no hay contacto).
- [x] La app no se queda colgada si el adaptador no responde.
- [x] Se puede salir con `START`.

## 8. Resultado en el vehículo (05/11/2026)

Primera ejecución real, con el contacto puesto y el motor parado:

```
IP PSP : 192.168.0.11
SSID   : TU_WIFI
Segur. : 0 (abierta)
Senal  : 100 %
Gateway: 192.168.0.10
TCP OK
ATI    : ELM327 v1.5
ATE0/ATL0/ATS0/ATH0/ATSP0 : OK
ATDPN  : A0
ATDP   : AUTO
ATRV   : 11.7V
0100   : 4100BE3EB811
010C   : 410C0000   -> 0 rpm
010D   : 410D00     -> 0 km/h
010B   : 410B61     -> 97 kPa (MAP, motor parado)
0105   : 410536     -> 14 C (refrigerante)
010F   : 410F39     -> 17 C (admisión)
```

Conclusiones:

- El enlace completo PSP → WiFi → ELM327 → ECU funciona y las fórmulas
  estándar devuelven valores plausibles con el motor parado.
- El clon **no identifica el protocolo**: `ATDPN` responde `A0` y `ATDP` solo
  `AUTO`. El formato de las respuestas es compatible con ISO 15765-4 CAN, pero
  queda pendiente confirmarlo (probar `ATSP6` y comprobar que sigue
  respondiendo).
- La máscara `BE 3E B8 11` confirma MAP, RPM, velocidad, ECT, IAT, carga,
  trims, avance, acelerador, sondas O2 y el bloque `0x20` (PIDs 21–40).
- **MAF (`0x10`) no está soportado** y `0x1F` tampoco. El caudal en L/h no se
  podrá medir por MAF; hará falta `0x5E` o quedará `N/D`.
- `ATRV` = 11.7 V con el contacto puesto es bajo. Antes de diagnosticar nada,
  cargar la batería: por debajo de ~12 V el ELM327 y la ECU pueden dar
  respuestas erráticas.

## 9. Qué falta después

Con la fase 0 cerrada, el siguiente paso es el núcleo portable:

- parser de líneas ELM327;
- decodificación de PID;
- scheduler;
- MAP/BOOST/baro;
- simulador con datos marcados `SIM`;
- tests en PC.

Después, el dashboard QV a 480×272.

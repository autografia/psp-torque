# Fase 1 — Núcleo portable y datos en vivo

Objetivo: separar toda la lógica OBD de la PSP, probarla en el PC y validarla en
el vehículo con una pantalla de texto. El dashboard gráfico QV se construye en
la fase 2 sobre este mismo núcleo.

> **Nota:** la Fase 2 ya está implementada. El dashboard gráfico sustituye la
> pantalla de texto; ver [`PSPGRAPHICS_MIGRATION.md`](PSPGRAPHICS_MIGRATION.md).
> Este documento se conserva como referencia del núcleo y de la validación de
> la Fase 1.

## 1. Núcleo portable

Todo el código de `include/obd/` y `src/core/` es C99 sin dependencias de PSP.
Se compila igual en el PC (pruebas) y en `psp-gcc` (EBOOT).

| Módulo | Archivos | Responsabilidad |
|---|---|---|
| Parser ELM327 | `elm327.c/.h` | Clasifica líneas: datos, `NO DATA`, error, `UNABLE TO CONNECT`, ruido, eco, ISO-TP multilínea. |
| PID y máscaras | `pid.c/.h` | Fórmulas estándar, bytes esperados, `0100`/`0120`, unidades internas. |
| Boost / baro | `boost.c/.h` | `BOOST = MAP − BARO`, prioridad `0x33` > captura key-on > fallback. |
| Scheduler | `scheduler.c/.h` | Periodos, prioridad, un PID en vuelo, 3 `NO DATA` desactivan. |
| Muestras | `samples.c/.h` | Almacén por PID con timestamp, origen (`REAL`, `EST`, `SIM`, `N/D`). |
| Perfil | `profile.c/.h` | Listas de sondeo QV y ENGINE, umbrales y baro de reserva. |

Reglas que el núcleo hace cumplir:

- Un PID sin respuesta **no** se convierte en `0`; se marca no disponible.
- `BOOST` no existe si no hay `MAP`; nunca se inventa.
- Si el baro es el fallback de 101.3 kPa, el boost se marca `EST`.
- La captura key-on solo se acepta al inicio de la sesión, con RPM válidas
  `< 50`, velocidad `< 1 km/h` y MAP entre 50 y 110 kPa.
- El PID `0x33`, si aparece, sustituye a la captura key-on y al fallback.

## 2. Pruebas en PC

```powershell
powershell -ExecutionPolicy Bypass -File tools\test_core.ps1
```

Se ejecuta en Docker con Alpine + gcc (`tools/test.Dockerfile`) para no depender
del compilador de la PSP. Cubre:

- Respuestas reales medidas en el MiTo (`410C0000`, `4100BE3EB811`, etc.).
- Fórmulas de RPM, MAP, ECT, IAT, carga, avance, trims, tensión, baro, MAF,
  caudal, aceite y distancia.
- Ruido, eco, `SEARCHING`, `NO DATA`, `?`, `BUS ERROR` y tramas ISO-TP.
- Prioridad y vencimientos del scheduler, desactivación por `NO DATA`,
  aplicación de máscaras reales y reactivación.
- Captura key-on, prioridad de `0x33`, baro no plausible y `EST`.
- Timestamps, muestras obsoletas e invalidación.

Resultado esperado: `OK: todas las pruebas pasan`.

## 3. App PSP de la fase 1

`src/psp/main.c` mantiene el patrón de red ya validado en la fase 0,
pero ahora la interfaz gráfica arranca antes que la red:

1. `gfx_init()` y la animación del trébol son lo primero que se ve; no
   se inicializa la pantalla de texto.
2. Carga módulos de red, `sceNetInit`, `sceNetInetInit`, `sceNetApctlInit`,
   `sceNetResolverInit` detrás del dashboard, con aviso gráfico.
3. Intenta conectar al perfil guardado (perfil 1) sin abrir el diálogo.
   El diálogo del sistema solo se abre si el usuario pulsa `SQUARE`.
4. Prueba destinos: líneas de `host.txt`, la puerta de enlace y
   `192.168.0.10:35000`.
5. Configura el ELM327 con `ATE0`, `ATL0`, `ATS0`, `ATH0`, `ATSP0`.
6. Lee `ATDPN`, `ATRV`, `0100` y, si el bloque `0x20` está presente, `0120`.
7. Aplica las máscaras al scheduler y sondea los PID del perfil QV más
   `0x11`, `0x04`, `0x0E` y `0x42`.
8. Dibuja el dashboard cada 250 ms y escribe telemetría en un log por
   sesión (`phase1_AAAAMMDD_HHMMSS.log`) cada segundo.
9. Termina solo con `START` (sin límite de tiempo). Ocho timeouts
   consecutivos se tratan como enlace perdido: el dashboard pasa a modo
   N/D y sigue abierto. `TRIANGLE` reintenta el perfil guardado y
   `SQUARE` abre la configuración WiFi de la PSP.

La app sigue siendo **solo lectura**: únicamente envía modo `01` y comandos
`AT` de configuración del adaptador. No hay modo `04`, `2E`, `2F` ni `31`.

## 4. Instalación

```powershell
powershell -ExecutionPolicy Bypass -File tools\build.ps1 -Clean
```

El script compila con `pspdev/pspdev` y deja en `dist\PSPTorqueOBD\`:

- `EBOOT.PBP`
- `host.txt`

Copiar ambos a `E:\PSP\GAME\PSPTorqueOBD\` con la PSP en modo USB (o usar
`tools\build.ps1`, que ya copia a `dist`; el traslado a la Memory Stick es
manual o con el explorador de Windows).

## 5. Ejecución en el coche

1. Encender el ELM327 y poner el contacto **con el motor parado**.
2. Abrir la app y conectar a tu red WiFi.
3. Esperar 5–10 s parado para capturar el baro key-on (`fuente KEY`).
4. Arrancar el motor y dejarlo al ralentí 30–60 s; acelerar suavemente para
   provocar MAP/BOOST.
5. `START` para cerrar la sesión y `START` otra vez para salir.
6. Reconectar USB y leer el log:

```powershell
powershell -ExecutionPolicy Bypass -File tools\read_log.ps1
```

## 6. Formato del log de sesión

```
PSP Torque OBD - fase 1
build: fase1 2026-11-05

net common: 0x...
...
> ATI
  ELM327 v1.5
> 0100
  4100BE3EB811
mascara 0100: BE3EB811
mascara 0120: ...
BARO key-on capturado: 97.0 kPa
t=1 rpm=850 map=42 boost=-0.55 baro=97(KEY) ect=40 iat=20 spd=0 rtt=45 ok=8 nd=0 to=0
...
Fin de sesion
PIDs activos: ...
Peticiones: ... datos: ... NO DATA: ... timeouts: ...
BARO: captura KEY-ON
RESULTADO: OK
```

## 7. Criterios de aceptación

- [x] El núcleo portable compila y pasa las pruebas en el PC.
- [x] El EBOOT de fase 1 compila sin errores con `pspdev`.
- [x] La app conserva el patrón de red y el diálogo netconf de la fase 0.
- [x] Solo se envían peticiones de lectura.
- [ ] En el coche, RPM/MAP se actualizan a ≥ 2 Hz y temperaturas a ≥ 1 Hz.
- [ ] `0120` se lee y la máscara se aplica al scheduler.
- [ ] `BOOST` tiene signo correcto y muestra `EST` solo si el baro es fallback.
- [ ] Un PID no soportado aparece como `N/D`.
- [ ] La sesión termina limpia con `START` y deja `RESULTADO: OK`.

## 8. Limitaciones conocidas

- Pantalla de texto de `pspDebugScreen`; sin gráficos ni trébol todavía.
- Sin reconexión automática: si se pierde el enlace, la sesión termina y hay
  que relanzar la app.
- Sesión limitada a 180 s para no bloquear pruebas largas.
- MAF (`0x10`) no está soportado por este vehículo; L/h queda `N/D` hasta
  confirmar `0x5E` en el bloque `0x20`.
- Si `0120` no responde, los PID 21–40 se quedan como desconocidos y el
  scheduler los desactiva tras tres `NO DATA`.

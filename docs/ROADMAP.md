# Roadmap y criterios de aceptación

## Fase 0 — Viabilidad técnica (bloqueante)

Objetivo: demostrar que la PSP puede hablar con el ELM327 WiFi y que el
toolchain compila un EBOOT.

Tareas:

1. Instalar `pspdev` (Docker o nativo) y compilar un “hello world” para PSP.
2. Probar en la PSP real: arranque, pantalla 480×272, lectura de botones,
   acceso a Memory Stick.
3. Configurar la red WiFi del adaptador y comprobar el modo de seguridad.
4. Implementar un cliente TCP mínimo en PSP (`sceNetApctl` + `sceNetInet`) y
   conectarlo al simulador ELM327 del PC.
5. Enviar `ATI`, `ATE0`, `ATSP0`, `0100`, `010C` y mostrar la respuesta cruda
   en pantalla.

Criterios de aceptación:

- [x] `EBOOT.PBP` arranca en la PSP objetivo.
- [x] La PSP obtiene IP (DHCP o estática) y abre TCP contra `192.168.0.10:35000`
      (o contra el simulador).
- [x] Se ve en pantalla una respuesta real de `0100` y `010C`.
- [x] Se puede desconectar y reconectar sin reiniciar la PSP.
- [x] Documentado el modo WiFi real del adaptador y el modelo/CFW de la PSP.

Resultado (05/11/2026): cerrada. PSP-2000/3000 con ARK4, red WiFi propia
abierta, IP `192.168.0.11`, `ATI`=`ELM327 v1.5`, `0100`=`4100BE3EB811`,
`RESULTADO: OK`. Ver [`FASE0.md`](FASE0.md).

Riesgos: WPA2 no soportado, adaptador sin DHCP, AP que no acepta el cliente PSP,
toolchain no disponible.

## Fase 1 — Núcleo OBD + datos en vivo

Estado: **cerrada**. Núcleo y app de texto implementados y validados en coche;
el dashboard grafico de la fase 2 sustituyo a la pantalla de texto.

Tareas:

1. [x] Núcleo portable: parser ELM327, PID estándar, muestras, scheduler,
   boost/baro.
2. [x] Tests de unidad del núcleo en PC (fórmulas, respuestas válidas, `NO DATA`,
   timeouts, líneas con ruido).
3. [x] Perfil `ALFA_MITO_QV_2010` compilado.
4. [x] UI QV gráfica (tacómetro, velocidad, boost, ECT, IAT, MAP, conexión).
5. [x] Animación de arranque del trébol.
6. [x] Simulador ELM327 con datos sintéticos marcados `SIM`.

Criterios de aceptación:

- [x] En el coche, RPM, velocidad, MAP, ECT e IAT se actualizan a una cadencia
      útil (objetivo ≥ 2 Hz para RPM/MAP; ≥ 1 Hz para temperaturas).
- [x] BOOST se calcula correctamente y muestra signo; `EST` visible si el baro
      es estimado.
- [x] Un PID no soportado muestra `N/D`, nunca `0` (garantizado por el núcleo).
- [x] La UI sigue respondiendo si se desenchufa el adaptador.
- [x] Reconexión automática con backoff.
- [x] Sin escrituras a la ECU.

La app de texto de esta fase y el detalle del núcleo están en
[`FASE1.md`](FASE1.md).

## Fase 2 — ENGINE MODE + TELEMETRÍA

Estado: **cerrada**. Dashboard grafico completo, con las siete pantallas, diez
temas, panel de telemetria editable y descubrimiento de centralitas. La
exportacion en formato CSV largo queda pendiente (el log de sesion actual es
un log de texto con una linea por muestreo).

Tareas:

1. Segunda pantalla de motor con presiones, temperaturas, aire, combustible y
   encendido.
2. STFT/LTFT, lambda/O2 solo si están disponibles.
3. Estadísticas de sesión (mín/máx/media/muestras).
4. CSV long-format con flush periódico y cierre seguro.
5. Gráficas con ring buffer y huecos.
6. Diagnóstico integrado (máscaras de PID, `ATDPN`, `ATRV`, respuestas crudas).

Criterios de aceptación:

- [ ] CSV abrible en Excel/LibreOffice, con timestamps y sin filas corruptas
      tras apagar la PSP. *Pendiente: hoy el log es texto plano.*
- [x] Las estadísticas coinciden con los valores vistos en pantalla.
- [x] Las gráficas no conectan huecos largos.
- [x] El diagnóstico distingue “PID no soportado” de “timeout”.
- [x] La sesión completa no supera el presupuesto de memoria.

## Fase 2.5 — Robustez de la capa OBD (FASE D)

Objetivo: que la sesion sobreviva a un adaptador que se apaga, a un bus que
se cae y a una reconexion, sin congelar la interfaz y sin mostrar datos
falsos.

Estado: **D1, D2 y D3 implementados** (`fase2.39`); pendiente de validacion
final en hardware.

1. [x] **D1 — Lectura OBD no bloqueante.** Una sola peticion en vuelo; el
   estado avanza una vez por iteracion del bucle. Nunca se envia un comando
   nuevo mientras hay otro pendiente y la GUI no se bloquea esperando.
2. [x] **D2 — Handshake validado.** Drenado del socket antes de cada
   secuencia AT, `ATE0` previo a `ATI` (para que el eco no contamine el
   nombre del adaptador) y reintento del handshake si `0100` devuelve la
   mascara `00000000`.
3. [x] **D3 — Reconexion no bloqueante.** La espera entre reintentos ocurre
   en el bucle principal (la GUI sigue viva); `SELECT` cancela una conexion
   en curso; si hay IP pero ningun ELM responde, se reconecta el AP y se
   reintenta el escaneo. El resumen de sesion es acumulativo: cuenta cortes
   de enlace y reconexiones, y ya no se reinicia al reconectar.
4. [ ] Validacion en hardware de los tres puntos con el vehiculo real.

Criterios de aceptacion:

- [x] Ninguna escritura a la ECU en ningun camino de recuperacion.
- [x] Un corte de enlace no congela la interfaz.
- [x] El resumen final refleja la sesion completa, no solo el ultimo tramo.
- [ ] Sin cortes de bus prolongados ni desincronizacion de respuestas tras
      reconectar (medido sobre el log de una sesion real en coche).

## Fase 3 — Parámetros específicos Alfa Romeo (investigación)

Tareas:

1. Leer VIN, CalID, CVN y nombre de ECU.
2. Recopilar documentación/exportaciones verificables.
3. Implementar modo 22 con cabecera explícita y ensamblado ISO-TP si hace falta.
4. Añadir PID extendidos solo con validación en el coche, marcados
   `experimental`.
5. Visor de peticiones/respuestas para validar antes de usar en el dashboard.

Criterios de aceptación:

- [ ] Cada PID extendido tiene fuente, petición, fórmula y fecha.
- [ ] Se ha validado en ralentí, carga y deceleración.
- [ ] Ningún PID inventado ni copiado sin contrastar.
- [ ] Los parámetros no validados siguen apareciendo como `N/D`.

## Fase 4 — Pulido

- Perfil guardable desde la PSP (config.ini).
- Ajuste de umbrales, redline, escala de boost y unidades.
- Gestión de energía y aviso de batería baja.
- Empaquetado (`EBOOT.PBP`, icono, manual).
- Pruebas de uso real: montaje, legibilidad con sol, guantes, acompañante.

## Fuera de alcance

- Escritura/codificación en la ECU.
- Borrado de DTC.
- Conexión simultánea con Torque/AlfaOBD.
- Bluetooth/USB en PSP.
- Android/iOS/PC como producto final (el simulador de PC es solo herramienta).

## Riesgos principales

| Riesgo | Impacto | Mitigación |
|---|---|---|
| PSP sin WPA2 nativo | Alto | Adaptador abierto/WEP/WPA-TKIP, plugin ARK4 o puente |
| Adaptador sin DHCP | Medio | IP estática configurable |
| PID no disponibles | Alto | Máscaras + `N/D`, perfil por sesión |
| ELM327 clon lento/inestable | Medio | Timeouts, backoff, diagnóstico |
| Ancho de banda bajo | Medio | Scheduler por prioridad, no pedir todo |
| Memoria/CPU PSP | Medio | Buffers fijos, 30 fps, atlas de fuente |
| Memory Stick lenta/extraída | Bajo | Flush periódico, cierre seguro |
| Interpretar boost/Torque mal | Alto | MAP/baro explícitos, `EST`, no inventar |
| Fatiga/seguridad al conducir | Alto | Montaje, acompañante, solo lectura |

## Decisiones pendientes

1. Modo de seguridad real del ELM327 y modelo/CFW de PSP.
2. Toolchain: Docker `pspdev` o instalación nativa.
3. Confirmar si el coche expone `0x0B` (MAP), `0x33` (baro), `0x5C` (aceite)
   y `0x5E` (caudal).
4. Confirmar el redline real y los umbrales que quiere el usuario.
5. Confirmar si se usará el plugin WPA2 de ARK4 o un puente.

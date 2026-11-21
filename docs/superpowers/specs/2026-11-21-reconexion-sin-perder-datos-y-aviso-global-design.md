# Diseño corto: reconexión sin perder datos + aviso global

- **Fecha:** 2026-11-21
- **Estado:** propuesto, pendiente de aprobación
- **Base:** `fase2.46` (commit `652538e`)
- **Tamaño:** cambio pequeño (dos mejoras localizadas en la capa PSP)

## Por qué

El usuario usa el dashboard **para ver datos al volante**. Lo que más le
molesta es que los datos desaparezcan. En los logs reales (`phase1_076`–
`080`) un STOPPED persistente provoca un corte y una reconexión WiFi de
10–30 s durante los cuales el panel queda en N/D.

Además los avisos de temperatura (ECT 105/115 °C, IAT 70 °C) solo se
pintan sobre la aguja del GENERAL: si estás en MOTOR o DIAG no los ves.
Tampoco hay aviso de batería baja, aunque la Fase 4 del ROADMAP lo pedía.

## Objetivos

1. Que una reconexión **no borre los últimos valores** de la pantalla.
2. Que un aviso de temperatura o voltaje se vea **en cualquier pestaña**.
3. No tocar la capa OBD ni la regla de no inventar lecturas.

## No objetivos

- No fijar el protocolo (`ATSP7`): rompería la universalidad con otro coche.
- No cambiar los umbrales de temperatura: ya están en `profile.h` y están
  validados en la ECU.
- No meter sonido nuevo (el aviso del AE86 es fijo, por decisión del proyecto).

## Aproximaciones consideradas

| | Qué | Por qué no |
|---|---|---|
| A | Subir el hueco a 12 ms para evitar STOPPED | Ya medimos que 8 ms funciona bien; no ataca el síntoma |
| B | `ATZ` antes de reconectar + conservar valores atenuados | **Elegida** |
| C | Fijar `ATSP7` | Rompe universalidad |
| D | Aviso solo en la aguja (ya existe) | Invisible en MOTOR/DIAG |

## Diseño

### Parte 1 — Reconexión sin perder datos

- **Conservar los valores**: cuando `link_bad` cierra el socket, **no se
  invalida el almacén de muestras** (`g_store`). Los últimos valores siguen
  ahí y `sample_value()` los devuelve mientras no pase su TTL (6 s los
  rápidos, 30 s aceite/combustible).
- **Atenuación durante la reconexión**: mientras `reconn_active`, el bucle
  llama a `draw_screen()` y ésta aplica `g_gfx_dim = 55` a todo el
  contenido (igual que ya hace con datos viejos). La cabecera y el cartel
  de red siguen a brillo completo, leyéndose `REINTENTANDO n/3`.
- **`ATZ` antes de tirar el WiFi**: hoy `link_bad` cierra el socket antes
  de llegar al bloque de reconexión. Se cambia el orden: al detectar el
  atasco **no se cierra aún**, se marca `reconn_active` y un flag
  `try_atz`. En el vencimiento del backoff se prueba un `ATZ` (reset del
  ELM327) sobre el socket existente, con timeout corto:
  - si contesta `OK` → se rehace solo el handshake sobre la misma
    conexión TCP, sin `sceNetApctlDisconnect()`;
  - si falla → se cierra el socket y se cae al camino de reconexión WiFi
    actual.
  Se escribe `Recuperacion: ATZ OK` o `ATZ fallo -> WiFi` en el log para
  poder medirlo. El `ATZ` es bloqueante (~1 s), pero ocurre cuando el
  panel ya está en modo recuperación.

### Parte 2 — Aviso global en la cabecera

- En `draw_top_bar()`, antes del estado de conexión, se calcula el aviso
  con prioridad: `CRIT` > `WARN`.
- Condiciones (valores ya definidos en `include/obd/profile.h`):
  - ECT ≥ 115 °C → `! REFRIGERANTE` en rojo, parpadeando.
  - ECT ≥ 105 °C → `! REFRIGERANTE` en ámbar.
  - IAT ≥ 70 °C → `! ADMISION` en ámbar.
  - voltaje < 11,5 V → `! BATERIA` en ámbar.
- Se muestra **en la cabecera**, a la izquierda del reloj de sesión. El
  estado de conexión (`ONLINE`/`ELM ATASCADO`/…) se desplaza un poco para
  dejar hueco. El aviso parpadea con el VBlank (una medida sencilla:
  `now/500 & 1`), sin temporizadores nuevos.
- Si hay dos avisos a la vez, gana el de mayor prioridad y al lado se pone
  `+1` para indicar que hay más.

## Verificación

- Build sin avisos, tests del core en verde.
- En la PSP: provocar un corte (apagar el ELM327) y comprobar que los
  valores siguen visibles atenuados con `REINTENTANDO n/3` encima.
- Simular aviso con un valor alto imposible de alcanzar en coche: se
  comprueba por código que el texto aparece en las cinco pestañas; en
  hardware se validará cuando el coche esté caliente.
- Log: nueva línea `Recuperacion: ...` para medir si el atajo `ATZ` funciona.

## Riesgos

- Si `ATZ` deja el adaptador en un estado peor, se pierde la conexión y se
  cae al WiFi de todas formas: reversible con un ajuste, y el log lo dice.
- La cabecera tiene ~480 px con 7 pestañas: el aviso + estado + reloj
  podrían no caber. Se mide con `gfx_text_width()` y se recorta el estado
  si hace falta; el aviso siempre tiene prioridad.

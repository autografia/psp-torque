# Diseño: FASE F — Suavidad del indicador y transparencia del panel

- **Fecha:** 2026-11-16
- **Estado:** propuesto, pendiente de aprobación
- **Build base:** `fase2.41` (commit `1b33668`)
- **Alcance:** dos vías independientes, Vía 1 (motor de datos) y Vía 2
  (transparencia del panel)

---

## 1. Por qué

El usuario conduce mirando el panel y el indicador "va a saltos": cuando acelera,
la aguja **da escalones grandes** (de ~1400 rpm de golpe) en vez de seguir el
coche. Confirmado por el usuario: el problema es el **tamaño del escalón**, no
que el dato llegue tarde.

La FASE E (fase2.40) quitó el retardo: el dato llega ~60 ms después de que el
coche cambió. La FASE C+A (fase2.41) subió la cadencia del RPM de 2,14 Hz a
3,00 Hz. Pero 3 Hz significa que entre dos muestras el motor puede subir
1500 rpm, y el suavizado actual apenas amortigua (α = 0,89 a 333 ms): el
escalón se ve entero.

### 1.1 Lo que dicen los logs (`phase1_071` … `phase1_074`)

| Magnitud | 071 (fase2.40) | 072 (fase2.41) |
|---|---|---|
| hueco entre comandos (`gap`) | ~8-24 ms | **0 ms** |
| `rtt` (envío → prompt) | 55,0 ms | **64,8 ms** |
| capacidad del enlace | 12,05 req/s | **9,25 req/s** |
| cadencia del RPM | 2,14 Hz | **3,00 Hz** |
| `STOPPED` | 0 | 0 en 072/074, **839 en 073** |

**El experimento C salió parcialmente negativo.** Con hueco 0 el ELM327 retrasa
su respuesta un tick de 16,7 ms (la distribución del prompt pasa de 50 ms a
66,7 ms). El hueco de ~24 ms **no era tiempo perdido**: era tiempo que el
adaptador necesitaba. Por eso la capacidad bajó de 12 a 9,25 req/s.

**Lo que sí funcionó** fue A (reparto del presupuesto): la demanda bajó de
16,03 a 12,05 req/s y el RPM pasó de 2,14 a 3,00 Hz.

### 1.2 Hallazgos adicionales

- **El ELM327 se queda pegado en `STOPPED`** (839 líneas en 073). No es un
  fallo puntual: es un estado del que no sale. El reintento a 100 ms no lo
  desbloquea y el estado persiste durante decenas de comandos.
- **La reconexión WiFi funciona pero es lenta y ciega**: 3 intentos, con
  "Sin IP" 2 veces, ~30 s sin datos.
- **El panel se congela en silencio**: `data_is_stale()` considera el dato
  fresco hasta **3000 ms**. Durante un corte de enlace los indicadores siguen
  enseñando el último valorKnown durante 3 s, sin decir nada.
- **El boost es correcto** (MAP 21→248 kPa, baro real 97 kPa → −0,76…+1,51 bar):
  el MiTo traba a vacío de verdad a ralenti. No es un error, pero un indicador
  que marca −0,8 en reposo parece averiado.

---

## 2. Objetivos

1. **Que la aguja se mueva en escalones pequeños.** Duplicar la cadencia del
   RPM (3 Hz → 5-6 Hz) reduce el escalón a la mitad sin tocar la regla de no
   inventar lecturas.
2. **Que el panel nunca se congele callado.** Si el dato es viejo, si el
   adaptador está atascado o se está reconectando, el panel lo dice.
3. **No romper nada de lo que ya funciona**: cero escrituras a la ECU, cero
   datos inventados, y el resto de las 40+ funciones intactas.

## 3. No objetivos (anti-objetivos)

- **No interpolar la aguja entre muestras.** Se descartó explícitamente: entre
  dos lecturas reales la aguja mostraría una estimación, y la regla del
  proyecto es "no se inventan lecturas".
- **No tocar el hardware.** El puente ESP32 y otro adaptador quedan fuera.
- **No cambiar el protocolo OBD.** El modo 01 no admite pedir varios PIDs en
  un comando; no hay atajo.
- **No subir el suavizado para tapar los escalones.** Eso reintroduce el
  retardo de 1,5 s que la FASE E eliminó. El suavizado se queda como está
  (τ = 150 ms).
- **No tocar la pantalla de INFO, MODULOS ni SNIFFER** más de lo necesario.

---

## 4. Vía 1 — Motor: el hueco mínimo seguro

### 4.1 Idea

El hueco entre comandos no es un defecto de nuestro bucle: es un requisito
del adaptador. La FASE C lo eliminó y la capacidad cayó. Lo que hay que hacer
es encontrar **el hueco mínimo que el ELM327 tolera** y quedarnos ahí.

Evidencia: con ~8-24 ms de hueco responde a 50 ms; con 0 ms responde a
66,7 ms. El umbral está entre 0 y ~8 ms.

### 4.2 Diseño

1. **Volver a un hueco con mínimo garantizado.** Se restaura la espera entre
   el fin de una respuesta y el envío del siguiente, pero ahora **de forma
   explícita y medible**: una constante `ELM_MIN_GAP_MS`.
2. **Hacer el hueco ajustable desde el menú de ajustes** (`HUECO ELM`), con
   valores 0 / 2 / 4 / 6 / 8 / 12 / 17 ms. Motivo: el umbral depende del
   adaptador concreto (clone, firmware, distancia, temperatura) y no se puede
   fijar a ciegas. Con el ajuste en vivo, **un solo viaje** da la curva
   completa: el log ya mide `gap`, `rtt`, los `STOPPED` y la cadencia.
3. **La métrica `gap=` se mantiene** en la línea de estado, y se le añade el
   hueco *efectivo* configurado para poder leer de un vistazo si el valor
   elegido se está respetando.

La enforcement es una espera **no bloqueante**: si todavía no ha pasado el
hueco mínimo, el bucle no envía y sigue (dibuja, lee el pad,etc.). No se
introduce ninguna espera bloqueante nueva.

### 4.3 Criterio de cierre

Se elige el hueco mínimo que cumpla **las tres** condiciones a la vez:

- `STOPPED` = 0 en la sesión.
- `rtt` medio ≤ 55 ms (es decir, sin deferral del adaptador).
- Cadencia del RPM ≥ 4,5 Hz.

Si ningún valor cumple las tres, se baja la exigencia de cadencia a 4 Hz y se
documenta el techo real del enlace con este adaptador.

### 4.4 Qué NO se hace aquí

- No se automatiza el ajuste (un lazo adaptativo que suba y baje el hueco
  según el `rtt` observado). Sería elegante, pero introduces oscilaciones y
  es imposible de validar en un viaje. Si el umbral resulta ser estable, se
  puede automatizar en una fase posterior.

---

## 5. Vía 2 — Transparencia: que el panel cuente la verdad

### 5.1 Idea

El peor momento del uso actual no es un número feo: es **un panel que se
queda quieto sin decir por qué**. Tres situaciones que hoy son silenciosas:

| Situación | Hoy | Propuesta |
|---|---|---|
| Dato viejo (enlace caído) | Frozen 3 s, luego "SIN DATOS" | **Los indicadores se atenúan** cuando el dato pasa de 1 s, y "SIN DATOS" a los 800 ms |
| Adaptador atascado en `STOPPED` | Indistinguible de "sin datos" | Estado **"ADAPTADOR ATASCADO"** en la cabecera |
| Reconectando | Cartel sobre el panel | Cartel + **"RECONECTANDO (2/3)"** con el intento actual |

### 5.2 Diseño

1. **Edad del dato.** Se calcula `now - ts` de la última muestra de RPM (o
   MAP) y se usa en dos sitios:
   - **Atenuación visual**: si la edad > 1 s, los gauges se dibujan con el
     color atenuado (mezcla con el fondo). El usuario ve "esto está viejo"
     sin leer un número.
   - **Umbral de `stale`**: baja de 3000 ms a **800 ms**, para que "SIN
     DATOS" aparezca cuando el dato ya no sirve, no 3 s después.
   - **Indicador numérico opcional**: el `N/D` ya existe; no se añade texto
     nuevo al panel principal para no cargarlo.
2. **Estado real del adaptador.** El ELM327 tiene tres estados observables
   desde la app: respondiendo, atascado en `STOPPED` (contador de STOPPED
   seguidos) y sin responder. Se expone un estado de 4 valores en la cabecera:
   `ONLINE` / `SIN DATOS` / `ATASCADO` / `DESCONECT`, con el punto de color
   correspondiente. El contador se reinicia cuando llega una respuesta
   buena.
3. **Reconexión con progreso.** El cartel de conexión pasa a incluir el
   intento actual (`RECONECTANDO 2/3`). Es un cambio de una línea en el texto
   del cartel.

### 5.3 Criterio de cierre

- Con el adaptador apagado, los indicadores se atenúan en < 1 s y el cartel
  explica qué pasa.
- Con el adaptador atascado (STOPPED), la cabecera dice `ATASCADO` en < 2 s.
- El usuario nunca ve un panel "congelado en silencio": o los datos están
  frescos, o el panel lo dice.

---

## 6. Restricciones

- **PSP 2000/3000, C99, PSPSDK.** Nada de APIs que no estén en el SDK.
- **Sin escrituras a la ECU**, en ningún camino.
- **Presupuesto de memoria**: buffers estáticos, nada de `malloc` en el bucle
  de render.
- **El render no puede bajar de 30 fps** con el enlace activo (es el diseño
  de la FASE B6: se Bajan los fps para no robarle tiempo al sondeo).
- La Vía 2 toca la capa de dibujo: el número de vértices por frame ya está
  cerca del límite (`v` máximo observado 22596 de 32768). La atenuación se
  implementa **cambiando el color**, no añadiendo geometría.

---

## 7. Alternativas consideradas

| Alternativa | Qué compra | Qué cuesta | Veredicto |
|---|---|---|---|
| **Vía 1 + Vía 2** (elegida) | Más cadencia (escalones más pequeños) + panel que explica los fallos | Un viaje de calibración | **Elegida** |
| Solo Vía 1 | Escalones más pequeños | El panel sigue mudo en los fallos | Incompleta |
| Solo Vía 2 | El panel explica los fallos | Los escalones siguen | Incompleta |
| Interpolar la aguja (Vía 3) | Movimiento continuo perfecto | Muestra una estimación entre lecturas | Descartada: choca con la regla del proyecto |
| Puente ESP32 | +40% de capacidad | HW nuevo | Fuera de alcance |
| Hueco adaptativo en runtime | Se ajusta solo a cada adaptador | Oscilaciones, no validable | Fuera de alcance (fase posterior) |

---

## 8. Verificación

### 8.1 Automática (núcleo, en PC)

- `test_link_budget`: la demanda del planificador sigue cabiendo en el
  presupuesto. **Si el hueco elegido sube la capacidad, la demanda puede
  bajar y este test se reajusta.**
- `test_smooth`: el retardo del filtro sigue siendo τ.
- Nuevo: test del **presupuesto de demanda** parametrizado por la capacidad
  medida, para que cambiar el hueco no obligue a reescribir el test a mano.

### 8.2 En hardware (PSP + coche)

| Métrica | Dónde | Objetivo |
|---|---|---|
| `STOPPED` | `grep -c` | **0** |
| `rtt` medio | líneas `t=` | ≤ 55 ms |
| `gap` efectivo | líneas `t=` | igual al configurado |
| Cadencia de `010C` | recuento / duración | **≥ 4,5 Hz** |
| Peticiones/s | recuento / duración | ≥ 14 |
| `STOPPED` seguidos | nuevo contador del log | 0 |
| fps / cpu / `v` | líneas `t=` | 30 fps, `cpu` ≤ 8 ms, `v` < 32766 |

### 8.3 Prueba de usuario

- **¿La aguja sigue el motor sin escalones grandes?** (el criterio real)
- Con el adaptador apagado: ¿el panel explica lo que pasa antes de 1 s?
- Con el adaptador atascado: ¿la cabecera lo dice?

---

## 9. Reparto del trabajo

**Fase F1 — Motor** (Vía 1)
- `ELM_MIN_GAP_MS` + enforcement no bloqueante en `src/psp/main.c`.
- Ajuste `HUECO ELM` en el menú (enum, dibujo, entrada, guardado/carga).
- Métricas nuevas en el log.
- Tests del núcleo.

**Fase F2 — Transparencia** (Vía 2)
- `data_is_stale()` a 800 ms + atenuación por color de los gauges.
- Estado `ATASCADO` en la cabecera (contador de STOPPED).
- Cartel de reconexión con el intento actual.

F1 y F2 son independientes: F2 no depende de F1 y se puede implementar en
paralelo o después.

---

## 10. Deuda técnica que deja esta fase

- El umbral del ELM327 queda como **constante ajustada a mano**, no
  autodetectado.
- El log mezcla datos de varias sesiones en el mismo fichero cuando hay
  reconexiones (el contador `t=` se reinicia). Debería llevar un sufijo de
  sesión.
- `PROFILE_ENGINE_POLL` sigue sin usarse (el planificador real se monta en
  `build_scheduler`).

# Tema visual — ALFA ROMEO QUADRIFOGLIO VERDE

## 1. Identidad

- **Nombre en pantalla**: `QV TELEMETRY`.
- **Concepto**: instrumentación deportiva italiana, fondo negro, rojo Alfa y
  verde trébol, sin recargar.
- **Resolución**: 480 × 272, orientación horizontal.
- **Trébol**: diseño **original y estilizado** de cuatro hojas, no el logotipo
  oficial. El proyecto es personal y no está afiliado a Alfa Romeo.

## 2. Paleta

| Uso | Color | Hex |
|---|---|---|
| Fondo principal | Negro | `#050505` |
| Fondo de panel | Negro carbón | `#101214` |
| Texto primario | Blanco cálido | `#F2F0EB` |
| Texto secundario | Gris | `#8C9196` |
| Rojo Alfa | Rojo | `#C8102E` |
| Verde trébol | Verde | `#008C45` |
| Acento italiano | Blanco | `#F2F0EB` |
| Aviso | Ámbar | `#FFB000` |
| Crítico | Rojo | `#FF2A2A` |
| OK / conexión | Verde | `#2ECC71` |
| Simulador | Cian | `#00C2D1` |

Reglas:

- El rojo no se usa como color de fondo dominante: es acento y aviso.
- El verde no se usa para “todo va bien” sin texto/icono; nunca solo color.
- Degradados suaves y líneas finas; evitar sombras costosas.

### 2.1 Temas seleccionables

La paleta es una estructura en tiempo de ejecución (`gfx_theme`). La ventana
de ajustes (SELECT) permite cambiar entre cinco presets:

| Tema | Acento principal | Acento secundario |
|---|---|---|
| `QV ROSSO` | Rojo Alfa `#C8102E` | Verde trébol `#008C45` |
| `VERDE` | Verde `#009A50` | Lima `#7AC143` |
| `AMBAR` | Naranja `#FF8C00` | Ámbar claro `#FFC42A` |
| `AZZURRO` | Azul `#008FC8` | Cian `#00C2D1` |
| `MONO` | Blanco `#E4E4E4` | Gris `#A8A8A8` |

`QV_CRIT` sigue siendo rojo en todos los temas para que los avisos críticos
no pierdan significado. El contraste tiene tres niveles (BAJO / NORMAL /
ALTO) y afecta a fondos, pistas y textos, no a los acentos.

Los ajustes se guardan en `ms0:/PSP/GAME/PSPTorqueOBD/settings.txt` y se
aplican al instante, también en el dashboard sin adaptador.

## 3. Tipografía

- **Dígitos**: fuente condensada de alta legibilidad, con cifras tabulares.
  Candidatas OFL: *Barlow Semi Condensed*, *Roboto Condensed*, *Inter*.
- **Etiquetas**: misma familia en versalitas o mayúsculas, tamaño pequeño.
- En PSP no se renderiza TTF en tiempo real: se genera un **atlas bitmap** con
  antialiasing (8/16/24/48/64 px) y se sube como textura. Los números grandes
  usan un atlas separado para mantener nitidez.
- Tamaños previstos:
  - valor principal RPM/velocidad: 56–72 px;
  - valores secundarios: 22–28 px;
  - etiquetas: 11–13 px;
  - unidad: 12–14 px, gris.

## 4. Estructura general

```
┌──────────────────────────────────────────────────────────────┐
│ QV   ENGINE   TELEMETRY        [WiFi] [ONLINE]   12:34  READ │  barra superior 22 px
├──────────────────────────────────────────────────────────────┤
│                                                              │
│                      área de contenido                       │
│                                                              │
├──────────────────────────────────────────────────────────────┤
│ BARO 1013 hPa EST · SESIÓN 00:12:43 · SOLO LECTURA           │  barra inferior 20 px
└──────────────────────────────────────────────────────────────┘
```

- Margen seguro: 6–8 px.
- Barra superior: pestañas de modo, estado de conexión, hora.
- Barra inferior: fuente de baro, tiempo de sesión, aviso `SOLO LECTURA`.
- El aviso `SIM` (si se usa el simulador) ocupa un badge cian visible en la
  barra superior y en el CSV.

## 5. Pantalla QV MODE

Objetivo: lectura en un vistazo, con RPM como protagonista.

```
┌──────────────────────────────────────────────────────────────┐
│ QV  ENGINE  TELEMETRY          [WiFi] ONLINE        12:34   │
│ ┌────────────────────┐   ┌────────────────────────────────┐  │
│ │                    │   │  128                        │  │
│ │     TACÓMETRO      │   │  km/h                       │  │
│ │     0–7000         │   └────────────────────────────────┘  │
│ │     RPM 3.250      │   ┌────────────────────────────────┐  │
│ │                    │   │ BOOST  +0.82 bar               │  │
│ └────────────────────┘   │ ▓▓▓▓▓▓▓▓▓▓░░░░░  (vacío/boost) │  │
│                          └────────────────────────────────┘  │
│ ┌──────────────┐ ┌──────────────┐ ┌───────────────────────┐  │
│ │ ECT   88 °C  │ │ IAT   34 °C  │ │ MAP  1.84 bar abs.    │  │
│ └──────────────┘ └──────────────┘ └───────────────────────┘  │
│ BARO 1013 hPa EST · SESIÓN 00:12:43 · SOLO LECTURA           │
└──────────────────────────────────────────────────────────────┘
```

- **Tacómetro circular** de ~180 px: arco de 0 a régimen máximo configurable
  (por defecto 7000 rpm, zona roja desde el redline configurado). Aguja o arco
  relleno; número de RPM en el centro.
- **Velocidad**: dígitos grandes, `km/h` debajo.
- **Boost**: barra horizontal centrada en 0, escala configurable (por defecto
  −1.0 a +1.5 bar). El valor digital lleva signo. Si es estimado, badge `EST`.
- **ECT / IAT**: dos tiles con valor y unidad. Si no hay dato, `N/D`.
- **MAP**: tile pequeño con valor absoluto en bar y kPa. Etiqueta `ABS` para no
  confundir con boost.
- **Conexión**: icono + texto; nunca solo color.

## 6. Pantalla ENGINE MODE

Dos subpáginas con `L/R` (o tocar las pestañas), indicador `1/2`.

### ENGINE A — presiones, temperaturas y aire

| Tile | Contenido |
|---|---|
| MAP | kPa y bar absolutos |
| BOOST | bar relativos, con signo |
| MAF | g/s |
| ECT | °C |
| IAT | °C |
| OIL T | °C o `N/D` |
| THROTTLE | % |
| LOAD | % |

### ENGINE B — combustible y encendido

| Tile | Contenido |
|---|---|
| STFT B1 | % |
| LTFT B1 | % |
| LAMBDA | λ o `N/D` (no derivar de banda estrecha) |
| O2 B1S1 | V / trim si existe |
| IGN ADV | ° |
| FUEL PRESS | kPa o `N/D` |
| FUEL RATE | L/h o `N/D` |
| BARO | kPa + fuente (`PID 33`, `KEY-ON`, `EST`) |

- Rejilla de 4 × 2 tiles (o 3 × 3 si se necesitan más), 150 × 52 px cada uno.
- Valor grande, etiqueta pequeña, unidad gris.
- Los tiles no disponibles se atenúan y muestran `N/D`; no desaparecen (así el
  usuario sabe que el parámetro se contempla pero no está disponible).

## 7. Pantalla TELEMETRY MODE

```
┌──────────────────────────────────────────────────────────────┐
│ QV  ENGINE  TELEMETRY     [REC 00:03:12]  [CSV: OK]          │
│ ┌──────────────────────────────────────────────────────────┐ │
│ │  gráfica 120 columnas · 3 series seleccionables           │ │
│ │  RPM / BOOST / ECT                                        │ │
│ └──────────────────────────────────────────────────────────┘ │
│ ┌─────────────────┐ ┌─────────────────┐ ┌────────────────┐  │
│ │ MÁX RPM 5.980   │ │ MÁX BOOST 1.12  │ │ MÁX ECT 94 °C  │  │
│ │ MÍN ...         │ │ ...             │ │ ...            │  │
│ └─────────────────┘ └─────────────────┘ └────────────────┘  │
│ BARO 1013 hPa EST · 00:12:43 · SOLO LECTURA                  │
└──────────────────────────────────────────────────────────────┘
```

- **Gráfica** de 120–240 columnas, con ring buffer de 600 muestras por serie.
  Se dibujan huecos cuando no hay datos; no se interpola a través de un
  `NO DATA` largo como si fuera real.
- **Series** seleccionables: RPM, BOOST, MAP, ECT, IAT, MAF, LOAD, STFT/LTFT.
  Colores: blanco, rojo, verde, ámbar; siempre con leyenda.
- **Estadísticas**: mínimo, máximo, media y número de muestras por sesión.
- **CSV**: indicador `REC` parpadeante lento y estado del archivo. Inicio/parada
  con `□`; confirmación al salir si se está grabando.
- **Reset** de sesión con confirmación (no borra el CSV ya escrito).

Formato CSV propuesto (long format, robusto ante PIDs ausentes):

```csv
timestamp_iso,t_ms,session_s,pid,name,value,unit,valid,source,raw
2026-11-05T18:12:43.120,123456,12.345,0x0C,RPM,3250,rpm,1,REAL,41 0C 32 90
2026-11-05T18:12:43.140,123476,12.365,0x0B,MAP,184,kPa,1,REAL,41 0B B8
```

`source`: `REAL`, `CALC`, `EST`, `SIM`, `UNAVAILABLE`. Los datos simulados
nunca se mezclan con una sesión real.

## 8. Animación de arranque

Duración total: **1,4–1,8 s**, saltable con `X`/`START`.

1. **0–250 ms**: negro.
2. **250–1000 ms**: el trébol se dibuja con cuatro hojas que crecen desde el
   centro; barrido de verde a rojo; leve rotación.
3. **1000–1300 ms**: aparece `QV TELEMETRY` y una línea fina roja/verde.
4. **1300–1500 ms**: fundido a la pantalla QV.

- 60 fps si el coste es bajo; si no, 30 fps.
- Sin datos falsos durante la animación: solo branding y estado `CONECTANDO`.
- El trébol se puede dibujar con primitivas (4 formas de corazón rotadas) o con
  una textura de 128×128; evitar depender de un PNG grande.
- No bloquear el arranque de red: la animación corre mientras se inicializa el
  WiFi.

## 9. Transiciones y estados

- Cambio de modo: fundido + desplazamiento de 10–12 px, 140–180 ms, ease-out.
- Aparición de valores: sin animación numérica que falsee la lectura; el valor
  se actualiza cuando llega.
- `CONECTANDO`: punto ámbar pulsante (1 Hz) + texto.
- `ONLINE`: punto verde + `OBD`.
- `SIN DATOS` / `STALE`: gris + texto; el último valor se atenúa y se marca.
- `ERROR`: rojo + texto; ofrece `X` para reintentar.
- `SIM`: badge cian permanente.

## 10. Avisos

| Condición | Aviso | Comportamiento |
|---|---|---|
| ECT ≥ 105 °C | Ámbar | Borde del tile + icono |
| ECT ≥ 115 °C | Rojo | Borde + texto `TEMP` |
| IAT ≥ 70 °C | Ámbar | Borde del tile |
| RPM ≥ redline − 300 | Ámbar | Arco del tacómetro |
| RPM ≥ redline | Rojo | Arco + `REDLINE` |
| Boost > umbral configurable | Ámbar | Barra |
| Tensión < 11,5 V (motor en marcha) | Ámbar | Tile de tensión (ENGINE) |

Los umbrales son configurables y genéricos; no se presentan como límites
oficiales del MiTo. El redline real debe confirmarse.

## 11. Unidades y formato

- Velocidad: `km/h`, entero.
- Temperaturas: `°C`, entero.
- MAP: `kPa` entero y `bar` con 2 decimales.
- BOOST: `bar` con 2 decimales y signo (`+0.82`, `−0.45`).
- Presión de combustible: `kPa` o `bar` según el PID.
- MAF: `g/s` con 1 decimal.
- Carga/acelerador/trims: `%` entero o 1 decimal.
- Lambda: `λ` con 2 decimales (solo si es medida real).
- Caudal: `L/h` con 1 decimal.
- Avance: `°` con 1 decimal.
- Tensión: `V` con 1 decimal.
- Hora: `HH:MM` local; si el RTC no es válido, `--:--`.

## 12. Rendimiento y detalles de implementación

- Fondo estático y marcos se pueden pre-renderizar en una textura o display
  list; solo se redibujan agujas, barras y texto.
- Arcos del tacómetro por segmentos; evitar cientos de primitivas por frame.
- Texto por atlas; no `printf` por frame.
- Cachear cadenas formateadas y solo regenerarlas cuando cambia el valor
  (comparar entero/centésimas).
- Sin transparencias a pantalla completa si reducen los fps.
- El estado `SIM` debe ser imposible de confundir con datos reales.

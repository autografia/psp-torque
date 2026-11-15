# Compatibilidad OBD-II — Alfa Romeo MiTo QV 2010

## 1. Vehículo de referencia

| Campo | Valor |
|---|---|
| Marca / modelo | Alfa Romeo MiTo Quadrifoglio Verde |
| Año | 2010 |
| Motor | 1.4 Turbo MultiAir, 170 CV |
| ECU | Original, sin reprogramar (código exacto a confirmar con documentación/VIN) |
| Adaptador | ELM327 WiFi |
| Software de referencia | Torque y AlfaOBD (compatibilidad ya comprobada por el usuario) |
| Perfil | `ALFA_MITO_QV_2010` |

**No se asume** que todos los PID estén disponibles. La disponibilidad se
descubre en cada sesión con las máscaras de PID soportados y con sondeos
individuales.

## 2. Enlace y protocolo

- La PSP se conecta por WiFi al AP del ELM327 y abre un socket TCP
  (`192.168.0.10:35000` por defecto; IP y puerto configurables).
- El ELM327 habla con la ECU. Para este coche lo esperable es **ISO 15765-4
  CAN de 11 bits a 500 kbps**, pero se confirma con:

```
ATDPN     -> p. ej. "A6" (auto, protocolo 6)
ATDP      -> "ISO 15765-4 (CAN 11/500)"
ATRV      -> tensión de batería vista por el adaptador
```

- Estrategia: `ATSP0` (auto) al arrancar. Si la autodetección es inestable, se
  puede fijar `ATSP6` en el perfil, pero solo después de confirmarlo.
- La app no necesita conocer la ECU para leer modo 01; el direccionamiento
  funcional estándar lo gestiona el ELM327.

## 3. PID estándar prioritarios

Fórmulas según SAE J1979 / ISO 15031-5. `A`, `B`, `C`, `D` son los bytes de
datos de la respuesta, en orden. La columna **Estado** indica lo que hay que
verificar en el coche: no es una promesa de disponibilidad.

| # | PID | Nombre | Bytes | Fórmula | Unidad | Estado |
|---|---|---|---|---|---|---|
| 1 | `0x0C` | RPM motor | 2 | `(256A+B)/4` | rpm | Esperado; verificar |
| 2 | `0x0D` | Velocidad | 1 | `A` | km/h | Esperado; verificar |
| 3 | `0x0B` | MAP (absoluta) | 1 | `A` | kPa | **Crítico**; verificar |
| 4 | `0x05` | Temp. refrigerante | 1 | `A−40` | °C | Esperado; verificar |
| 5 | `0x0F` | Temp. admisión | 1 | `A−40` | °C | Esperado; verificar |
| 6 | `0x11` | Acelerador | 1 | `A×100/255` | % | Verificar |
| 7 | `0x04` | Carga calculada | 1 | `A×100/255` | % | Verificar |
| 8 | `0x10` | MAF | 2 | `(256A+B)/100` | g/s | Verificar |
| 9 | `0x0E` | Avance de encendido | 1 | `A/2−64` | ° | Verificar |
| 10 | `0x06` | STFT banco 1 | 1 | `A/1.28−100` | % | Verificar |
| 11 | `0x07` | LTFT banco 1 | 1 | `A/1.28−100` | % | Verificar |
| 12 | `0x14`–`0x1B` | Sondas O2 (banda estrecha) | 2 | V: `A/200`; trim: `B/1.28−100` | V / % | Verificar |
| 13 | `0x33` | Presión barométrica | 1 | `A` | kPa | **Clave para BOOST**; verificar |
| 14 | `0x5C` | Temp. aceite | 1 | `A−40` | °C | Si existe |
| 15 | `0x0A` | Presión de combustible | 1 | `A×3` | kPa | Si existe |
| 16 | `0x22` | Presión rail relativa al colector | 2 | `(256A+B)×0.079` | kPa | Si existe |
| 17 | `0x23` | Presión rail (manómetro) | 2 | `(256A+B)×10` | kPa | Si existe |
| 18 | `0x42` | Tensión módulo | 2 | `(256A+B)/1000` | V | Verificar |
| 19 | `0x49` | Pedal acelerador D | 1 | `A×100/255` | % | Verificar |
| 20 | `0x4A` | Pedal acelerador E | 1 | `A×100/255` | % | Verificar |
| 21 | `0x2F` | Nivel de combustible | 1 | `A×100/255` | % | Verificar |
| 22 | `0x5E` | Caudal de combustible | 2 | `(256A+B)/20` | L/h | Si existe |
| 23 | `0x1F` | Tiempo desde arranque | 2 | `256A+B` | s | Verificar |
| 24 | `0x21` | Distancia con MIL encendido | 2 | `256A+B` | km | Verificar |
| 25 | `0x31` | Distancia desde borrado de DTC | 2 | `256A+B` | km | Verificar |

### 3.1 Máscaras de PID soportados

Antes de sondear, pedir:

```
0100   -> 41 00 [4 bytes]   PIDs 0x01–0x20
0120   -> 41 20 [4 bytes]   PIDs 0x21–0x40
0140   -> 41 40 [4 bytes]   PIDs 0x41–0x60
0160   -> 41 60 [4 bytes]   PIDs 0x61–0x80
0180, 01A0... si el vehículo responde
```

Cada bit indica soporte. Si el bit de `0x0B` no está, MAP no está disponible
por modo 01 y **no se debe inventar** un boost.

### 3.3 Disponibilidad medida en el MiTo QV (05/11/2026)

Primera lectura real, contacto puesto y motor parado:

```
0100 -> 41 00 BE 3E B8 11
```

| Estado | PIDs |
|---|---|
| Soportados | `01, 03, 04, 05, 06, 07, 0B, 0C, 0D, 0E, 0F, 11, 13, 14, 15, 1C, 20` |
| No soportados | `02, 08, 09, 0A, 10, 12, 16, 17, 18, 19, 1A, 1B, 1D, 1E, 1F` |
| Pendiente | `0120` (PIDs 21–40), `0140`, `0902` (VIN) |
| Soportados (21–40) | `21, 2F, 33, 40` |
| No soportados (21–40) | resto del bloque (22, 23, 24, 25, 26, 27, 28, 29, 2A, 2B, 2C, 2D, 2E, 30, 31, 32, 34, 35, 36, 37, 38, 39) |
| Soportados (41–60) | `41, 42, 51, 52` |
| No soportados (41–60) | resto del bloque, incluidos `5C` y `5E` |
| Pendiente | `0160`, `0902` (VIN) |

Lecturas con motor parado:

| PID | Crudo | Valor | Comentario |
|---|---|---|---|
| `0x0C` | `410C0000` | 0 rpm | Correcto |
| `0x0D` | `410D00` | 0 km/h | Correcto |
| `0x0B` | `410B61` | 97 kPa | MAP ≈ presión atmosférica; candidato a baro key-on |
| `0x05` | `410536` | 14 °C | Refrigerante frío |
| `0x0F` | `410F39` | 17 °C | Admisión fría |

Consecuencias para el diseño:

- **MAF no disponible** (`0x10`): no se puede calcular L/h desde MAF.
- **Caudal de combustible no disponible** (`0x5E`): `0140` confirma que `0x5E`
  no está soportado. L/h queda `N/D` hasta contar con otra fuente.
- **Temperatura de aceite no disponible** (`0x5C`): no soportado según `0140`.
- **Presión barométrica disponible** (`0x33`): ~97 kPa, fuente preferida para
  BOOST; no hace falta fallback `EST` en este vehículo.
- **Nivel de combustible disponible** (`0x2F`): porcentaje del depósito.
- **Tensión del módulo disponible** (`0x42`): voltios de la ECU.
- `0x1F` (tiempo desde arranque) no está; no usarlo para estadísticas.
- El adaptador es un clon `ELM327 v1.5` que responde `ATDPN=A0` y
  `ATDP=AUTO` (no identifica el protocolo). No asumir `ATSP6` sin probarlo.

### 3.2 Identificación (solo lectura)

```
0902   VIN
0904   Calibration ID
0906   CVN
090A   ECU name
```

Útiles para el perfil y para la investigación posterior. No se envía nada de
escritura.

## 4. MAP y BOOST

### 4.1 Definiciones

| Magnitud | Qué es | Unidad de trabajo |
|---|---|---|
| MAP | Presión absoluta del colector | kPa (y BAR absoluto en UI) |
| BARO | Presión atmosférica | kPa |
| BOOST | Presión relativa de sobrealimentación | BAR |

```
BOOST_bar = (MAP_kPa − BARO_kPa) / 100
```

- A ralentí, un motor de gasolina suele tener MAP < BARO → boost negativo
  (vacío). Se muestra el signo; no se recorta a cero.
- Bajo carga, MAP > BARO → boost positivo.
- `1 bar = 100 kPa`. Se muestran ambos: MAP en kPa/BAR absolutos y BOOST en BAR
  relativos, con etiquetas claras para no confundirlos.

### 4.2 Obtención de BARO

Orden de preferencia:

1. **PID `0x33`** si está soportado y el valor es plausible (50–110 kPa).
2. **Captura key-on**: con el contacto puesto y el motor parado, se toma MAP
   como presión atmosférica. Se guarda para la sesión y se etiqueta `KEY-ON`.
   Debe hacerse antes de arrancar; si el motor ya está en marcha, no es válido.
3. **Valor configurable**: por defecto `1013 hPa`, ajustable entre 900 y
   1050 hPa. Se etiqueta `EST` en la UI y en el CSV.

Si no hay MAP, BOOST = `N/D`. Si no hay baro real ni captura key-on, BOOST se
calcula con el valor configurable pero se muestra `EST` de forma visible.

### 4.3 Los dos escenarios de Torque

El usuario aún debe verificar qué muestra Torque:

- **Escenario 1 — Torque muestra MAP**: la app lee MAP por `0x0B` y calcula
  BOOST con su propio baro. Es el escenario preferido.
- **Escenario 2 — Torque muestra BOOST pero no MAP**: puede ser un PID
  propietario o un cálculo interno de Torque. La app **no adivina** ese PID.
  Opciones:
  - confirmar si MAP está disponible por `0x0B`;
  - obtener la definición exacta del PID personalizado de Torque (petición,
    cabecera, bytes, fórmula) y añadirla como PID extendido verificado;
  - mientras no se valide, mostrar BOOST como `N/D`.

No se puede asumir que el “boost” que enseña Torque sea un dato medido: en
muchos casos es `MAP − baro` calculado por la app.

## 5. Temperaturas, lambda y combustible

- **Refrigerante**: PID `0x05`. Rango normal orientativo 80–100 °C. Umbral de
  aviso y crítico configurables (por defecto 105 °C y 115 °C). No son valores
  específicos del MiTo; son avisos genéricos.
- **Admisión**: PID `0x0F`. En motor turbo puede subir con la carga; aviso
  configurable (por defecto 70 °C).
- **Aceite**: PID `0x5C` si existe. No hay PID estándar de **presión de aceite**;
  solo se implementará si se documenta un modo 22 fiable para esta ECU.
- **Lambda**: los PID `0x14`–`0x1B` son sondas de banda estrecha; su tensión
  **no es lambda**. No se debe presentar `0.45 V` como “lambda 1.0”. Si el coche
  expone una sonda wideband por `0x24`–`0x2B`, se implementará con la fórmula
  SAE correspondiente y validación en el coche; si no, `N/D`.
- **STFT/LTFT**: PID `0x06`/`0x07` (banco 1). Positivo = la ECU añade
  combustible (mezcla pobre); negativo = lo reduce (mezcla rica). Se muestran
  como porcentaje, no como lambda.
- **Caudal de combustible**: PID `0x5E` (L/h) si existe. Estimación a partir de
  MAF solo si el usuario la activa y se marca `EST`; nunca se presenta como
  medida.
- **Presión de combustible**: `0x0A`, `0x22`, `0x23` si existen. No se asume
  que el MiTo QV las exponga.

## 6. Parámetros específicos de Alfa Romeo

### 6.1 Lo que se sabe

- El MiTo QV responde a OBD-II estándar (Torque funciona).
- AlfaOBD accede a módulos y parámetros de fabricante, pero **no existe una
  lista pública estandarizada** de PID propietarios para esta ECU.
- El modo 22 (lectura de datos por identificador) es el mecanismo habitual,
  pero requiere el identificador, la cabecera CAN y la fórmula correctos.

### 6.2 Lo que no se hará

- No inventar direcciones CAN, identificadores de ECU, comandos `ATSH`/`ATSW`
  ni PID “mágicos”.
- No copiar listas de foros sin validación en el vehículo.
- No enviar comandos de escritura, activación de actuadores ni calibración.
- No hacer escaneos agresivos de CAN en el coche.

### 6.3 Procedimiento para añadir un PID extendido

Un PID extendido solo entra en el perfil si cumple **todos** estos puntos:

1. Fuente identificable (documentación, exportación de Torque/AlfaOBD, log
   propio).
2. Petición exacta (modo 22 + ID), cabecera si aplica, y bytes esperados.
3. Fórmula y unidades documentadas.
4. Validación en el MiTo real: valor plausible en ralentí, carga y deceleración.
5. Fecha, versión y estado `experimental` en el perfil.

Mientras no se cumpla, el parámetro aparece como `N/D` y el perfil lo marca
como no soportado.

### 6.4 Vía de investigación

- Leer VIN/CalID/CVN con modo 09.
- Revisar exportaciones de Torque (PID personalizados) y registros de AlfaOBD
  del propietario.
- Contrastar con documentación pública de la ECU (a confirmar; no asumir que
  la ECU es Bosch/Marelli sin verificar).
- Implementar primero un **visor de diagnóstico** que muestre peticiones y
  respuestas crudas; así se valida cualquier PID antes de usarlo en el
  dashboard.

## 7. Verificación en el vehículo (checklist)

Antes de dar por buena la matriz:

- [x] `ATI`, `ATDPN`, `ATDP`, `ATRV`. (`ATI`=ELM327 v1.5, `ATDPN`=A0, `ATDP`=AUTO, `ATRV`=11.7 V)
- [x] `0100` = `BE3EB811`, `0120` = `80022001`, `0140` = `C0C00000`. Falta `0160`.
- [ ] `010C`, `010D`, `010B`, `0105`, `010F`, `0111`, `0104`, `0110`. (los cinco primeros verificados; `0x10` no soportado)
- [ ] `0106`, `0107`, `0114`–`011B`, `0133`, `015C`, `015E`.
- [ ] `010A`, `0122`, `0123` (solo si los bits están activos).
- [ ] `0902`, `0904`, `0906`, `090A`.
- [ ] Comprobar MAP con contacto y motor parado (key-on) para baro.
- [ ] Comprobar MAP/BOOST a ralentí, aceleración y deceleración.
- [ ] Confirmar si Torque muestra MAP, BOOST o ambos y de dónde salen.
- [ ] Confirmar temperatura de aceite y presión de aceite (¿modo 22?).
- [ ] Confirmar régimen de corte/redline real para el indicador.
- [ ] Anotar versiones de firmware del ELM327 y de la ECU.

## 8. Límites y seguridad

- La velocidad de actualización real depende del ELM327 y de la ECU; no se
  prometen 20 Hz.
- El adaptador WiFi suele aceptar **un solo cliente**. No usar Torque y la PSP
  a la vez.
- El ELM327 puede tardar en entrar en modo bajo consumo; si se apaga, la app
  debe reconectar y avisar.
- No dejar el adaptador conectado de forma permanente: consume batería.
- La pantalla no debe manejarse mientras se conduce.

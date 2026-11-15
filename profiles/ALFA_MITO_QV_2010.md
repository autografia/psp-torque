# Perfil `ALFA_MITO_QV_2010`

Perfil por defecto del proyecto. Los valores marcados como **provisional** o
**por verificar** no deben tratarse como datos confirmados del vehículo.

## 1. Identidad

| Campo | Valor |
|---|---|
| `profile_id` | `ALFA_MITO_QV_2010` |
| `display_name` | `MiTo QV 2010` |
| `make` | Alfa Romeo |
| `model` | MiTo Quadrifoglio Verde |
| `year` | 2010 |
| `engine` | 1.4 Turbo MultiAir |
| `power_cv` | 170 |
| `displacement_cc` | 1368 |
| `fuel` | Gasolina |
| `ecu` | Original, sin reprogramar |
| `vin` | (leer con `0902`; no hardcodear) |
| `engine_code` | Por verificar con documentación/VIN |

## 2. Conexión

| Campo | Valor por defecto | Notas |
|---|---|---|
| `transport` | WiFi TCP | |
| `adapter_ip` | `192.168.0.10` | Configurable; algunos usan otra IP |
| `adapter_port` | `35000` | Alternativas: `23`, `2000` |
| `dhcp` | sí | Si falla, IP estática `192.168.0.11/24` |
| `protocol` | `ATSP0` (auto) | Confirmar con `ATDPN`; posible `ATSP6` |
| `read_timeout_ms` | 400 | Ajustable |
| `reconnect_backoff` | 1, 2, 5, 10 s | |
| `elm_init` | `ATZ`, `ATE0`, `ATL0`, `ATS0`, `ATH0`, `ATSP0` | |

## 3. Unidades

- Velocidad: `km/h`
- Temperatura: `°C`
- MAP: `kPa` + `bar` absolutos
- BOOST: `bar` relativos
- MAF: `g/s`
- Caudal: `L/h`
- Presión de combustible: `kPa` (o `bar` si el PID es de alta presión)
- Avance: grados
- Trims: `%`
- Lambda: `λ`
- Tensión: `V`

## 4. Presión barométrica

| Estrategia | Orden | Etiqueta UI |
|---|---|---|
| PID `0x33` | 1 | `PID 33` |
| Captura key-on (motor parado) | 2 | `KEY-ON` |
| Valor configurable | 3 | `EST` |

- `baro_fallback_hpa`: `1013` (rango 900–1050).
- `baro_min_kpa`: `50`, `baro_max_kpa`: `110` (plausibilidad).
- Si no hay MAP: BOOST = `N/D`.

## 5. PID y cadencia

### QV MODE

| Parámetro | PID | Periodo | Prioridad | Disponibilidad |
|---|---|---|---|---|
| RPM | `0x0C` | 100 ms | alta | Esperado; verificar |
| MAP | `0x0B` | 100 ms | alta | Crítico; verificar |
| Velocidad | `0x0D` | 200 ms | media | Esperado; verificar |
| ECT | `0x05` | 1000 ms | baja | Esperado; verificar |
| IAT | `0x0F` | 1000 ms | baja | Esperado; verificar |
| BARO | `0x33` | 5000 ms | baja | Si existe |
| BOOST | — | derivado | — | Requiere MAP + baro |

### ENGINE MODE

| Parámetro | PID | Periodo | Notas |
|---|---|---|---|
| MAP | `0x0B` | 150 ms | |
| BOOST | — | derivado | |
| MAF | `0x10` | 250 ms | Si existe |
| ECT | `0x05` | 1000 ms | |
| IAT | `0x0F` | 1000 ms | |
| Aceite | `0x5C` | 2000 ms | Si existe |
| Acelerador | `0x11` | 200 ms | |
| Carga | `0x04` | 250 ms | |
| STFT B1 | `0x06` | 500 ms | |
| LTFT B1 | `0x07` | 1000 ms | |
| Lambda/O2 | `0x14`–`0x1B` o `0x24`–`0x2B` | 500 ms | Solo si existe |
| Avance | `0x0E` | 250 ms | |
| Presión combustible | `0x0A`/`0x22`/`0x23` | 1000 ms | Si existe |
| Caudal | `0x5E` | 1000 ms | Si existe |
| Tensión | `0x42` | 2000 ms | |
| Pedal D/E | `0x49`/`0x4A` | 500 ms | Si existe |
| Nivel combustible | `0x2F` | 5000 ms | Si existe |

La cadencia es un objetivo, no una garantía. El scheduler se adapta si el
ELM327 responde lento.

## 6. Visual / umbrales (provisionales)

| Ajuste | Valor por defecto | Nota |
|---|---|---|
| `redline_rpm` | 6500 | **Verificar** en el coche/documentación |
| `tach_max_rpm` | 7000 | |
| `boost_min_bar` | −1.0 | Vacío |
| `boost_max_bar` | +1.5 | Ajustable |
| `ect_warn_c` | 105 | Genérico |
| `ect_crit_c` | 115 | Genérico |
| `iat_warn_c` | 70 | Genérico |
| `oil_warn_c` | 130 | Si hay sensor |
| `voltage_warn_v` | 11.5 | Motor en marcha |
| `rpm_warn_margin` | 300 | Antes del redline |
| `baro_fallback_hpa` | 1013 | |

Estos umbrales no son límites oficiales del MiTo; son avisos configurables.

## 7. Registro

| Campo | Valor |
|---|---|
| `logging_enabled` | sí, manual |
| `format` | CSV long |
| `path` | `ms0:/PSP/GAME/PSPTorqueOBD/logs/` |
| `filename` | `qv_YYYYMMDD_HHMMSS.csv` |
| `flush` | cada 1 s o 32 filas |
| `sim_data` | nunca en sesión real; badge `SIM` |

## 8. Configuración persistente (borrador INI)

```ini
[profile]
id=ALFA_MITO_QV_2010
name=MiTo QV 2010

[connection]
ip=192.168.0.10
port=35000
dhcp=1
protocol=AUTO
timeout_ms=400

[units]
speed=kmh
temp=c
map=kpa
boost=bar
fuel=lph

[baro]
fallback_hpa=1013
min_kpa=50
max_kpa=110

[display]
redline_rpm=6500
tach_max_rpm=7000
boost_min_bar=-1.0
boost_max_bar=1.5
ect_warn_c=105
ect_crit_c=115
iat_warn_c=70

[logging]
enabled=1
flush_rows=32
```

## 9. Pendiente de verificar en el vehículo

- [ ] Código de motor y ECU exactos.
- [ ] Protocolo real (`ATDPN`). (el clon responde `A0`/`AUTO`; pendiente forzar `ATSP6`)
- [x] MAP (`0x0B`) disponible: 97 kPa con contacto y motor parado.
- [ ] BARO (`0x33`) disponible. (pendiente `0120`)
- [ ] Temperatura de aceite (`0x5C`) disponible.
- [ ] Presión de aceite: ¿existe un modo 22 documentado?
- [x] MAF (`0x10`): **no soportado** según `0100`.
- [ ] Lambda wideband (`0x24`–`0x2B`) disponible.
- [ ] Presión de combustible (`0x0A`/`0x22`/`0x23`). (`0x0A` no soportado)
- [ ] Redline real.
- [ ] Qué muestra exactamente Torque (MAP, BOOST o ambos).
- [ ] Modo de seguridad WiFi del adaptador.
- [ ] Modelo de PSP y CFW.

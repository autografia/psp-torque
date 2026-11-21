# Reconexión sin perder datos + aviso global — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Que una pérdida de enlace no borre los valores de la pantalla y que los avisos de temperatura/voltaje se vean en cualquier pestaña.

**Architecture:** La decisión de alerta se va al núcleo portable (`obd_alert`) para poder testearla; la capa PSP conserva el almacén de muestras, atenúa el contenido durante la reconexión y prueba `ATZ` antes de tirar el WiFi. No se toca la capa de sondeo ni el parser.

**Tech Stack:** C99, PSPSDK (pspdev), tests del núcleo con gcc (`tools/test_core.ps1`), build de PSP con `tools/build_wsl.sh`.

**Spec:** `docs/superpowers/specs/2026-11-21-reconexion-sin-perder-datos-y-aviso-global-design.md`

## Global Constraints

- Solo lectura: ningún camino escribe en la ECU. `ATZ` resetea el **adaptador**, no la ECU: está permitido.
- No inventar lecturas: los valores durante la reconexión son los **últimos recibidos**, no estimaciones. La atenuación lo indica.
- Umbrales exactos (ya en `include/obd/profile.h`): ECT aviso 105 °C, crítico 115 °C; IAT aviso 70 °C; voltaje aviso < 11,5 V.
- C99 con `-Wall -Wextra -Werror` en los tests; 0 avisos en el build de PSP.
- Sin `malloc` en el bucle de render; sin APIs fuera de PSPSDK.
- El aviso se dibuja en la cabecera, que ya se protege a sí misma del factor `g_gfx_dim` (`draw_top_bar` guarda y restaura el valor).

## Review Focus

1. **Dos avisos a la vez** → gana el de mayor prioridad y se indica que hay más. Test: `test_alert_priority` en Task 1.
2. **Dato inválido (PID no soportado)** → no puede disparar aviso. Test: `test_alert_invalid_data` en Task 1.
3. **Umbral exacto** → 105,0 dispara aviso; 104,9 no. Test: `test_alert_thresholds` en Task 1.
4. **Reconexión con el almacén lleno** → los valores siguen visibles atenuados, nunca `N/D` inventado. Sin seam de test unitario; se comprueba en la PSP con el log y el checklist de la Task 2.
5. **`ATZ` sobre un socket muerto** → debe fallar limpiamente y caer al WiFi, sin colgar el bucle. Sin seam de test; timeout corto documentado en la Task 2 y comprobación en hardware.

---

### Task 1: Decisión de avisos en el núcleo

**Files:**
- Create: `include/obd/alert.h`
- Create: `src/core/alert.c`
- Create: `tests/test_alert.c`
- Modify: `tests/Makefile`, `tests/test_main.c`

**Interfaces:**
- Consumes: `PROFILE_ECT_WARN_C`, `PROFILE_ECT_CRIT_C`, `PROFILE_IAT_WARN_C` de `include/obd/profile.h`.
- Produces:
  - `obd_alert_level` (`OBD_ALERT_NONE=0`, `OBD_ALERT_WARN=1`, `OBD_ALERT_CRIT=2`).
  - `obd_alert_kind` (`OBD_ALERT_NONE_K=0`, `OBD_ALERT_COOLANT=1`, `OBD_ALERT_INTAKE=2`, `OBD_ALERT_BATTERY=3`).
  - `typedef struct { uint8_t level; uint8_t kind; uint8_t more; } obd_alert;`
  - `obd_alert obd_alert_eval(float ect_c, int ect_valid, float iat_c, int iat_valid, float volt, int volt_valid);`
  - `const char *obd_alert_text(uint8_t kind);` → `"REFRIGERANTE"` / `"ADMISION"` / `"BATERIA"` / `""`.

- [ ] **Step 1: Escribir los tests que fallan**

`tests/test_alert.c`:

```c
#include "obd/alert.h"
#include "test_util.h"

/* Sin datos validos no hay aviso: un PID no soportado nunca puede
 * disparar una alarma. */
void test_alert_invalid_data(void)
{
    obd_alert a = obd_alert_eval(0, 0, 0, 0, 0, 0);
    CHECK(a.level == OBD_ALERT_NONE);
    CHECK(a.kind == OBD_ALERT_NONE_K);
    CHECK(a.more == 0);
}

/* El umbral es inclusivo y exacto. */
void test_alert_thresholds(void)
{
    CHECK(obd_alert_eval(104.9f, 1, 0, 0, 12.5f, 1).level == OBD_ALERT_NONE);
    CHECK(obd_alert_eval(105.0f, 1, 0, 0, 12.5f, 1).kind == OBD_ALERT_COOLANT);
    CHECK(obd_alert_eval(114.9f, 1, 0, 0, 12.5f, 1).level == OBD_ALERT_WARN);
    CHECK(obd_alert_eval(115.0f, 1, 0, 0, 12.5f, 1).level == OBD_ALERT_CRIT);
    CHECK(obd_alert_eval(0, 0, 70.0f, 1, 12.5f, 1).kind == OBD_ALERT_INTAKE);
    CHECK(obd_alert_eval(0, 0, 69.9f, 1, 12.5f, 1).level == OBD_ALERT_NONE);
    CHECK(obd_alert_eval(0, 0, 0, 0, 11.5f, 1).level == OBD_ALERT_NONE);
    CHECK(obd_alert_eval(0, 0, 0, 0, 11.4f, 1).kind == OBD_ALERT_BATTERY);
}

/* CRIT gana a WARN; 'more' indica que habia otro aviso activo. */
void test_alert_priority(void)
{
    obd_alert a = obd_alert_eval(108.0f, 1, 72.0f, 1, 12.5f, 1);
    CHECK(a.level == OBD_ALERT_WARN);
    CHECK(a.kind == OBD_ALERT_COOLANT);
    CHECK(a.more == 1);

    a = obd_alert_eval(116.0f, 1, 72.0f, 1, 11.0f, 1);
    CHECK(a.level == OBD_ALERT_CRIT);
    CHECK(a.kind == OBD_ALERT_COOLANT);
    CHECK(a.more == 1);

    a = obd_alert_eval(95.0f, 1, 72.0f, 1, 11.0f, 1);
    CHECK(a.kind == OBD_ALERT_INTAKE);
    CHECK(a.more == 1);

    CHECK(strcmp(obd_alert_text(OBD_ALERT_COOLANT), "REFRIGERANTE") == 0);
    CHECK(strcmp(obd_alert_text(OBD_ALERT_NONE_K), "") == 0);
}
```

Declarar las tres funciones en `test_main.c` y llamarlas antes de `test_smooth()`. Añadir `../src/core/alert.c` a `SRC` en `tests/Makefile`.

- [ ] **Step 2: Ejecutar y ver que fallan**

Run: `powershell -ExecutionPolicy Bypass -File tools/test_core.ps1`
Expected: FAIL por `obd/alert.h` no encontrado (el test incluye la cabecera que aún no existe).

- [ ] **Step 3: Implementar el módulo**

`include/obd/alert.h`: tipos, constantes y las dos funciones. `src/core/alert.c`:
`obd_alert_eval` cuenta los avisos activos (ECT crítico, ECT aviso, IAT, batería), elige el de mayor severity con orden coolant > intake > battery entre iguales, y pone `more = (activos > 1)`. `obd_alert_text` devuelve el literal exacto de arriba.

- [ ] **Step 4: Ejecutar y ver que pasan**

Run: `powershell -ExecutionPolicy Bypass -File tools/test_core.ps1`
Expected: `OK: todas las pruebas pasan`.

- [ ] **Step 5: Commit**

```bash
git add include/obd/alert.h src/core/alert.c tests/test_alert.c tests/test_main.c tests/Makefile
git commit -m "F2: decision de avisos (ECT/IAT/bateria) en el nucleo portable"
```

---

### Task 2: Reconexión sin perder datos + aviso en la cabecera

**Files:**
- Modify: `src/psp/main.c` (bloque `link_bad`, bloque `reconn_active`, `draw_screen`, `draw_top_bar`)

**Interfaces:**
- Consumes: `obd_alert_eval`, `obd_alert_text` de la Task 1; `g_gfx_dim`, `g_store`, `g_sched`, `connect_network_and_elm`, `build_scheduler`, `net_ui_set`, `data_age_ms`.
- Produces: ninguna interfaz nueva hacia fuera; estado interno `static int g_try_atz`.

- [ ] **Step 1: Conservar el almacén y marcar la reconexión**

En el bloque `if (link_bad)`:
- **No** llamar a `obd_store_init(&g_store)` (no se hace hoy, pero se añade un comentario explícito para que nadie lo añada).
- Si el atasco es por STOPPED (`g_elm_stuck >= ELM_STUCK_RECOVER`), **no cerrar el socket**: poner `g_try_atz = 1` y dejar `sock` abierto. Para los otros motivos (8 timeouts, 40 errores) se cierra como hoy.
- Añadir `g_try_atz = 0;` en los caminos de reconexión manual y de `connect_network_and_elm`.

- [ ] **Step 2: Atenuar durante la reconexión**

En `draw_screen`, justo donde hoy se calcula `g_gfx_dim` desde la edad:

```c
g_gfx_dim = (reconn_active || g_net_busy) ? 55 :
            (age_ms >= DATA_OLD_MS) ? 55 : 100;
```

La cabecera y el cartel de red ya se dibujan con `g_gfx_dim = 100` temporal, así que siguen legibles.

- [ ] **Step 3: Intentar `ATZ` antes del WiFi**

En el bloque `if (reconn_active ...)`, una vez vencido `reconn_next` y antes de `connect_network_and_elm`:

```c
if (g_try_atz && sock >= 0) {
    g_try_atz = 0;
    elm_reply r;
    elm_send_and_read(sock, "ATZ", 1500, &r);
    if (r.bytes > 0 && !r.stopped) {
        log_file_printf("Recuperacion: ATZ OK\n");
        log_sync();
        /* Rehacer handshake sobre la misma conexion. */
        sock = connect_network_and_elm(targets, &target_count, 0);
        if (sock >= 0) {
            reconn_active = 0;
            reconnects++;
            g_elm_stuck = 0;
            g_last_resp_ms = 0;
            last_status = now_ms();
            continue;   /* ya no hace falta reconectar WiFi */
        }
    } else {
        log_file_printf("Recuperacion: ATZ fallo -> WiFi\n");
        log_sync();
        sceNetInetClose(sock);
        sock = -1;
    }
}
```

Nota CRITICA: `connect_network_and_elm()` hace `obd_store_init(&g_store)` y
`boost_init(&g_boost, ...)` al entrar (main.c:2508-2509). Llamarla tal cual
borraria los valores que queremos conservar. Por eso, justo antes de la
llamada se guardan ambos estados por valor (son structs pequeños y
estaticos, sin malloc):

```c
obd_sample_store saved_store = g_store;
boost_state saved_boost = g_boost;
int saved_link = g_link_connected;
sock = connect_network_and_elm(targets, &target_count, 0);
if (sock >= 0) {
    g_store = saved_store;
    g_boost = saved_boost;
    g_link_connected = saved_link;
    ...
}
```

`connect_network_and_elm` rehace el handshake y el `build_scheduler` con
las mascaras nuevas; solo se restauran los **ultimos valores medidos**,
no las mascaras. El ahorro frente al camino normal esta en que no se
llama a `sceNetApctlDisconnect()`/`sceNetApctlConnect(1)`.

- [ ] **Step 4: Aviso en la cabecera**

En `draw_top_bar`, justo después de calcular `dot`/`state`/`state_color`:

```c
obd_alert al = obd_alert_eval(ect, ect_valid, iat, iat_valid,
                              volt, volt_valid);
```

Los nombres `ect`/`ect_valid`/`iat`/... son los parametros nuevos de
`draw_top_bar`. La firma pasa a:

```c
static void draw_top_bar(int connected, int stale, uint32_t age_ms,
                         float ect, int ect_valid,
                         float iat, int iat_valid,
                         float volt, int volt_valid)
```

y `draw_screen` llama pasando `d.ect_val`, `d.ect_valid`, `d.iat_val`,
`d.iat_valid`, `d.volt_val`, `d.volt_valid`.

Si `al.level != OBD_ALERT_NONE`:
- construir `char warn[32]`: `"! %s%s"`, `obd_alert_text(al.kind)`,
  y `" +1"` si `al.more`;
- color `QV_CRIT` si level CRIT, `QV_AMBER` si WARN;
- parpadeo: si `((now_ms()/500u) & 1u) == 0`, dibujar `warn` en `x+16, y+8`;
  si no, no dibujarlo (el hueco queda limpio, sin guiones);
- desplazar el bloque `state + reloj` a la derecha si no cabe: calcular
  `gfx_text_width(warn,1)` y usar `right_x = GFX_W - 8 - warnwidth - 8 -
  gfx_text_width(righttop,1)` cuando haya aviso; si aun así `right_x < 350`,
  recortar el estado a las 4 primeras letras.

- [ ] **Step 5: Compilar y verificar**

Run: `powershell -Command "wsl -d Debian -e sh /mnt/d/psp_torqueOBD/tools/build_wsl.sh"`
Expected: sin `warning` ni `error`.
Run: `powershell -ExecutionPolicy Bypass -File tools/test_core.ps1`
Expected: `OK: todas las pruebas pasan`.

- [ ] **Step 6: Commit**

```bash
git add src/psp/main.c
git commit -m "Reconexion sin borrar datos y aviso global en la cabecera"
```

---

### Task 3: Build tag, documentación e instalación

**Files:**
- Modify: `src/psp/main.c` (`BUILD_TAG`), `docs/ROADMAP.md`, `README.md`, `README_EN.md`, `RUN.txt`

**Interfaces:**
- Consumes: todo lo anterior.
- Produces: `BUILD_TAG = "fase2.47 2026-11-21 reconexion sin perder datos y avisos globales"`.

- [ ] **Step 1: BUILD_TAG** con el valor exacto de arriba.

- [ ] **Step 2: Documentar** en el ROADMAP una fase 2.12 con lo implementado, y en los README (sección de funciones) una línea: "avisos de temperatura/voltaje visibles en todas las pantallas" y "los últimos valores permanecen atenuados durante la reconexión".

- [ ] **Step 3: RUN.txt**, en la sección de estados, añadir:
  `REINTENTANDO n/3: reconectando; los valores se mantienen atenuados.`

- [ ] **Step 4: Build, tests, instalar y commit**

```bash
powershell -Command "wsl -d Debian -e sh /mnt/d/psp_torqueOBD/tools/build_wsl.sh"
powershell -ExecutionPolicy Bypass -File tools/test_core.ps1
cp dist/PSPTorqueOBD/EBOOT.PBP "E:/PSP/GAME/PSPTorqueOBD/EBOOT.PBP"
git add -A && git commit -m "FASE 2.12: documentacion y build fase2.47"
```

- [ ] **Step 5: Verificar la instalación**

Comparar SHA-256 de `E:/PSP/GAME/PSPTorqueOBD/EBOOT.PBP` y
`dist/PSPTorqueOBD/EBOOT.PBP`; deben ser idénticos. Anotar el hash.

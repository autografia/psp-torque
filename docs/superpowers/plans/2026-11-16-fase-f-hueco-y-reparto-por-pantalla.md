# FASE F (F1 + F3 + F2.5) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Que la aguja del tacómetro no dé escalones grandes (más cadencia de RPM repartida según la pantalla visible) y que el panel explique siempre qué está pasando.

**Architecture:** Tres cambios independientes sobre `fase2.42` (commit `b858788`):
F2.5 (edad del dato global robusta), F1 (hueco mínimo del adaptador, ajustable en caliente para calibrar) y F3 (reparto del ancho de banda por pantalla activa). El núcleo (`src/core/`) gana la tabla de periodos y las funciones de consulta; la capa PSP (`src/psp/main.c`) decide qué PID está visible en cada pantalla.

**Tech Stack:** C99, PSPSDK (pspdev), núcleo portable testeado con gcc en Docker, GUI con `pspgu`.

**Spec:** `docs/superpowers/specs/2026-11-16-fase-f-suavidad-y-transparencia-design.md`

## Global Constraints

- **Solo lectura**: ningún camino escribe en la ECU, ni borra DTC, ni codifica.
- **No inventar lecturas**: un PID no disponible es `N/D`; nada de valores estimados sin marcar.
- **C99 estricto**, `-Wall -Wextra -Werror` en los tests del núcleo; el binario de PSP compila con `-Wall -Wextra` sin avisos.
- **Sin `malloc`** en el bucle de render; buffers y tablas estáticas.
- **Presupuesto de vértices**: `VBUF_VERTS 32768`; saturaba en 32766. La atenuación de F2.5 cambia **colores**, no geometría.
- **PSP 2000/3000**: nada de APIs fuera del PSPSDK.
- **Los nombres de widgets y los rótulos de la interfaz van en español en el original y traducidos vía la tabla `tr()` de `src/psp/main.c`** (la tabla tiene las entradas ES/EN; añadir un rótulo nuevo obliga a añadir las dos).
- **El texto visible del usuario va sin acentos en los literales de código** (el proyecto usa ASCII en los literales; los acentos solo en comentarios y documentación).

## Review Focus

Cinco entradas o modos de fallo que la spec implica pero que ningún test exercise todavía, ordenados por probabilidad de morder a un usuario real:

1. **Cambiar de pantalla con una petición OBD en vuelo.** El planificador se reprograma en el cambio de modo; si se resetean los plazos mientras hay una petición en vuelo, el PID en vuelo puede quedar marcado como "en vuelo" para siempre y el panel se queda sin datos. *Test: tras reprogramar, `obd_sched_next` sigue devolviendo un PID y el contador `in_flight` vuelve a -1.*
2. **Una pantalla sin PIDs visibles (MODULOS, SNIFFER, INFO).** El conjunto visible sería vacío y ningún PID quedaría "en pantalla": el planificador no debe quedarse sin nada que pedir ni vaciar la pantalla de golpe. *Test: la demanda de esas pantallas es la del conjunto base, no cero.*
3. **Un PID configurado por el usuario en un hueco o en una serie (p. ej. `W_STFT` en el GENERAL).** Si el PID visible no está en el conjunto base, su periodo debe ser el base y no el ×4. *Test: un PID visible fuera de la tabla base conserva su periodo base.*
4. **El cambio de hueco/serie en caliente (editor de vista).** El usuario cambia qué se ve; el conjunto visible cambia y hay que reprogramar. Si no se reprograma, el dato nuevo no llega hasta su periodo lento. *Test: reprogramar tras cambiar un hueco deja el PID pendiente de inmediato.*
5. **Timeout de 500 ms con `CMD_TIMEOUT_MS`.** Un timeout puede retrasar el RPM más de 1 s y la edad global se dispara. Con la edad global robusta el panel no debe avisar por un PID lento si otros fluyen. *Test: la edad global es la del último PID recibido, no la del RPM.*

---

### Task 1: Edad del dato global robusta (F2.5)

**Files:**
- Modify: `src/psp/main.c` (función `data_age_ms`, ~línea 3450)
- Test: `tests/test_main.c`

**Interfaces:**
- Consumes: `obd_store_get(const obd_sample_store *, uint8_t pid)` de `include/obd/samples.h`; `obd_sample.ts_ms`, `obd_sample.valid`.
- Produces: `static uint32_t data_age_ms(uint32_t now)` — edad en ms de la muestra **más reciente de cualquier PID válido**; `0xFFFFFFFF` si no hay ninguna. Sustituye a la versión que miraba solo RPM/MAP.

- [ ] **Step 1: Escribir el test que falla**

En `tests/test_main.c`, añadir una función `test_data_age` que Needs el almacén de muestras. El núcleo no tiene el `g_store` de la PSP, así que el test va sobre `obd_sample_store` directamente:

```c
#include "obd/samples.h"

/* La edad global del panel es la de la muestra mas reciente de
 * CUALQUIER PID: si el RPM se retrasa pero el resto fluye, el panel
 * esta vivo y no debe avisar. */
static void test_data_age(void)
{
    obd_sample_store st;
    obd_store_init(&st);

    /* Sin muestras: edad desconocida. */
    CHECK(obd_store_newest_ts(&st) == 0u);

    obd_store_set(&st, 0x0C, 1000.0f, UNIT_RPM, 1000u, OBD_SRC_REAL);
    obd_store_set(&st, 0x0B, 30.0f, UNIT_KPA, 1500u, OBD_SRC_REAL);

    /* La mas reciente manda, no la del RPM. */
    CHECK(obd_store_newest_ts(&st) == 1500u);

    /* Una muestra invalida no cuenta. */
    obd_store_invalidate(&st, 0x0B, 1600u);
    CHECK(obd_store_newest_ts(&st) == 1000u);
}
```

Registrar `test_data_age()` en `main()` antes de `test_link_budget()`.

- [ ] **Step 2: Ejecutar el test y ver que falla**

Run: `powershell -ExecutionPolicy Bypass -File tools/test_core.ps1`
Expected: FAIL con "implicit declaration of function 'obd_store_newest_ts'" (el test compila con `-Werror`, así que falla en compilación).

- [ ] **Step 3: Implementar `obd_store_newest_ts` en el núcleo**

En `include/obd/samples.h`, declarar:

```c
/* Marca de tiempo de la muestra valida mas reciente del almacen, o 0 si
 * no hay ninguna. */
uint32_t obd_store_newest_ts(const obd_sample_store *store);
```

En `src/core/samples.c`, implementarla recorriendo `store->items[0..count-1]` y quedándose con el `ts_ms` mayor entre las que tengan `valid` a 1.

- [ ] **Step 4: Usarla en `data_age_ms`**

En `src/psp/main.c`, cambiar el cuerpo de `data_age_ms` para que use `obd_store_newest_ts(&g_store)` en lugar de mirar solo RPM y MAP:

```c
static uint32_t data_age_ms(uint32_t now)
{
    if (g_demo) {
        return 0;
    }
    uint32_t ts = obd_store_newest_ts(&g_store);
    if (ts == 0u) {
        return 0xFFFFFFFFu;
    }
    return (uint32_t)(now - ts);
}
```

- [ ] **Step 5: Ejecutar los tests y ver que pasan**

Run: `powershell -ExecutionPolicy Bypass -File tools/test_core.ps1`
Expected: `OK: todas las pruebas pasan`.

- [ ] **Step 6: Commit**

```bash
git add src/psp/main.c src/core/samples.c include/obd/samples.h tests/test_main.c
git commit -m "F2.5: edad del dato global robusta (la muestra mas reciente de cualquier PID)"
```

---

### Task 2: Tabla de periodos base por PID (F3, núcleo)

**Files:**
- Modify: `include/obd/profile.h`, `src/core/profile.c`
- Test: `tests/test_main.c`

**Interfaces:**
- Consumes: nada de tareas anteriores.
- Produces:
  - `uint16_t obd_profile_base_period(uint8_t pid)` — periodo base del PID en ms, o 0 si el PID no está en la tabla.
  - `uint16_t obd_profile_period(uint8_t pid, int visible)` — `visible` distinto de 0 → periodo base; si no → base × 4, con tope de 30000 ms.
  - `int obd_profile_knows_pid(uint8_t pid)` — 1 si el PID está en la tabla.

- [ ] **Step 1: Escribir el test que falla**

En `tests/test_main.c`, añadir:

```c
/* FASE F3: los periodos base son los de "el PID esta en pantalla". Fuera
 * de pantalla se multiplican por 4 (tope 30 s) para no gastar ancho de
 * banda en lo que no se mira. */
static void test_profile_periods(void)
{
    /* El RPM es el dato que se mira en el tacometro: 200 ms (5 Hz). */
    CHECK(obd_profile_base_period(0x0C) == 200);
    CHECK(obd_profile_period(0x0C, 1) == 200);
    CHECK(obd_profile_period(0x0C, 0) == 800);

    /* MAP: 400 ms en pantalla, 1600 fuera. */
    CHECK(obd_profile_base_period(0x0B) == 400);
    CHECK(obd_profile_period(0x0B, 0) == 1600);

    /* Tope de 30 s para los muy lentos. */
    CHECK(obd_profile_period(0x33, 0) == 30000);

    /* Los PID de diagnostico en pantalla son rapidos (1 s): compiten por
     * el enlace solo cuando no se estan viendo. */
    CHECK(obd_profile_base_period(0x06) == 1000);
    CHECK(obd_profile_base_period(0x21) == 2000);

    /* PID desconocido: 0. */
    CHECK(obd_profile_base_period(0x77) == 0);
    CHECK(!obd_profile_knows_pid(0x77));
}
```

- [ ] **Step 2: Ejecutar el test y ver que falla**

Run: `powershell -ExecutionPolicy Bypass -File tools/test_core.ps1`
Expected: FAIL por funciones no declaradas.

- [ ] **Step 3: Implementar la tabla en el núcleo**

En `include/obd/profile.h`, añadir las declaraciones de las tres funciones y un comentario con la tabla.

En `src/core/profile.c`, añadir la tabla. **Estos son los valores exactos:**

```c
/* Periodo base de cada PID: el que usa cuando esta en la pantalla
 * activa. Fuera de pantalla se multiplica por 4. */
static const struct { uint8_t pid; uint16_t period_ms; } g_base_periods[] = {
    { 0x0C, 200 },   /* RPM */
    { 0x0B, 400 },   /* MAP */
    { 0x0D, 800 },   /* velocidad */
    { 0x11, 800 },   /* mariposa */
    { 0x49, 800 },   /* pedal */
    { 0x4A, 800 },   /* pedal (posicion E) */
    { 0x04, 1500 },  /* carga */
    { 0x05, 3000 },  /* refrigerante */
    { 0x0F, 3000 },  /* admision */
    { 0x06, 1000 },  /* STFT */
    { 0x07, 1000 },  /* LTFT */
    { 0x0E, 1000 },  /* avance */
    { 0x14, 1000 },  /* O2-1 */
    { 0x15, 1000 },  /* O2-2 */
    { 0x21, 2000 },  /* distancia con MIL */
    { 0x42, 5000 },  /* voltaje */
    { 0x5C, 10000 }, /* aceite */
    { 0x2F, 10000 }, /* combustible */
    { 0x33, 30000 }  /* baro */
};
```

`obd_profile_base_period` busca el PID en la tabla y devuelve su periodo o 0.
`obd_profile_period(pid, visible)`: si `visible` → el base; si no → `base * 4`, y si el resultado supera 30000, 30000.
`obd_profile_knows_pid(pid)`: 1 si está en la tabla.

- [ ] **Step 4: Ejecutar los tests y ver que pasan**

Run: `powershell -ExecutionPolicy Bypass -File tools/test_core.ps1`
Expected: `OK: todas las pruebas pasan`.

- [ ] **Step 5: Commit**

```bash
git add include/obd/profile.h src/core/profile.c tests/test_main.c
git commit -m "F3: tabla de periodos base por PID en el nucleo"
```

---

### Task 3: Demanda por pantalla dentro del presupuesto (F3, núcleo)

**Files:**
- Modify: `include/obd/profile.h`, `src/core/profile.c`, `tests/test_main.c`

**Interfaces:**
- Consumes: `obd_profile_base_period` de la Task 2.
- Produces:
  - `float obd_profile_demand(uint8_t visible_mask)` — demanda en req/s del conjunto de PIDs cuya máscara de visibilidad coincide. La máscara de visibilidad de cada PID la define `obd_profile_visible_mask(uint8_t pid)` (bit 0 = GENERAL, bit 1 = MOTOR, bit 2 = DIAG, bit 3 = TELEM).
  - `uint8_t obd_profile_visible_mask(uint8_t pid)`.
  - `float obd_profile_demand_all(void)` — demanda si TODO se pidiera a la vez (el peor caso).

- [ ] **Step 1: Escribir el test que falla**

En `tests/test_main.c`, añadir:

```c
/* La demanda de cada pantalla tiene que caber en la capacidad del
 * enlace. El reparto por pantalla es lo que permite subir el RPM a
 * 5 Hz sin que el resto se quede sin ancho de banda. */
static void test_demand_by_screen(void)
{
    float d_general = obd_profile_demand(0x01);
    float d_motor   = obd_profile_demand(0x02);
    float d_diag    = obd_profile_demand(0x04);
    float d_telem   = obd_profile_demand(0x08);

    printf("  demanda: general %.2f  motor %.2f  diag %.2f  telem %.2f"
           "  (todo %.2f) req/s\n",
           d_general, d_motor, d_diag, d_telem, obd_profile_demand_all());

    CHECK(d_general <= PROFILE_LINK_BUDGET_REQ_S);
    CHECK(d_motor   <= PROFILE_LINK_BUDGET_REQ_S);
    CHECK(d_diag    <= PROFILE_LINK_BUDGET_REQ_S);
    CHECK(d_telem   <= PROFILE_LINK_BUDGET_REQ_S);
    CHECK(obd_profile_demand_all() <= PROFILE_LINK_BUDGET_REQ_S);

    /* El RPM tiene que ser el mas rapido de todos. */
    CHECK(obd_profile_base_period(0x0C) <=
          obd_profile_base_period(0x0B));

    /* Una pantalla sin PIDs propios (MODULOS, SNIFFER, INFO) no puede
     * quedarse sin nada que pedir: cuenta con el conjunto base. */
    CHECK(obd_profile_demand(0x10) > 0.0f);
    CHECK(obd_profile_demand(0x10) <= PROFILE_LINK_BUDGET_REQ_S);
}
```

**Nota sobre `obd_profile_demand(0x10)`**: la máscara `0x10` no corresponde a
ninguna pantalla de datos. La función debe interpretarla como "conjunto
base" (los PID que la tabla marca como visibles en alguna pantalla), que es
exactamente lo que necesitan MODULOS, SNIFFER e INFO. El test lo fija para que
esa pantalla no se quede nunca sin peticiones.

- [ ] **Step 2: Ejecutar el test y ver que falla**

Run: `powershell -ExecutionPolicy Bypass -File tools/test_core.ps1`
Expected: FAIL por funciones no declaradas.

- [ ] **Step 3: Implementar el reparto**

En `src/core/profile.c`, ampliar la tabla anterior con la máscara de visibilidad:

```c
static const struct {
    uint8_t pid;
    uint16_t period_ms;
    uint8_t screens;   /* bit0 GENERAL, bit1 MOTOR, bit2 DIAG, bit3 TELEM */
} g_base_periods[] = {
    { 0x0C, 200,   0x0F },  /* RPM: en todas */
    { 0x0B, 400,   0x03 },  /* MAP: general y motor */
    { 0x0D, 800,   0x0B },  /* velocidad: general, motor y telem */
    { 0x11, 800,   0x07 },  /* mariposa: general, motor, telem */
    { 0x49, 800,   0x03 },  /* pedal: general y motor */
    { 0x4A, 800,   0x03 },  /* pedal (posicion E) */
    { 0x04, 1500,  0x03 },  /* carga: general y motor */
    { 0x05, 3000,  0x0F },  /* refrigerante: en todas */
    { 0x0F, 3000,  0x0F },  /* admision: en todas */
    { 0x06, 1000,  0x04 },  /* STFT: solo DIAG */
    { 0x07, 1000,  0x04 },  /* LTFT: solo DIAG */
    { 0x0E, 1000,  0x04 },  /* avance: solo DIAG */
    { 0x14, 1000,  0x04 },  /* O2-1: solo DIAG */
    { 0x15, 1000,  0x04 },  /* O2-2: solo DIAG */
    { 0x21, 2000,  0x04 },  /* distancia: solo DIAG */
    { 0x42, 5000,  0x0E },  /* voltaje: motor, DIAG y telem */
    { 0x5C, 10000, 0x02 },  /* aceite: solo motor */
    { 0x2F, 10000, 0x06 },  /* combustible: motor y DIAG */
    { 0x33, 30000, 0x01 }   /* baro: solo general */
};
```

`obd_profile_visible_mask(pid)` devuelve el byte `screens` del PID (0 si no está en la tabla).
`obd_profile_demand(mask)`: suma, para cada PID de la tabla, `1000.0f / periodo` si su máscara tiene un bit en común con `mask`, y `1000.0f / off_period` en caso contrario. Los PID cuya máscara **no** incluye ninguna pantalla (por ejemplo un PID que solo se muestra en un hueco configurado) cuentan con su periodo base en todas.
`obd_profile_demand_all()`: suma `1000.0f / periodo base` de todos los PID de la tabla.

**Nota sobre el presupuesto**: con estos valores, la demanda de la pantalla GENERAL sale en ~11,8 req/s y la de "todo a la vez" en ~26 req/s. Por eso `PROFILE_LINK_BUDGET_REQ_S` **debe subir a 27** y el test `test_link_budget` (que exige que el "todo a la vez" quepa) **debe reescribirse** para exigir en su lugar que **cada pantalla** quepa en un presupuesto de 13 req/s. Los dos presupuestos son constantes distintas:

```c
/* Capacidad medida del enlace con el hueco minimo de 8 ms: ~13 req/s.
 * Cada pantalla tiene que caber en esto. */
#define PROFILE_LINK_BUDGET_REQ_S 13.0f
```

El test `test_link_budget` existente se **sustituye** por `test_demand_by_screen` (esta tarea) y se elimina la exigencia de que "todo a la vez" quepa, porque con el reparto por pantalla nunca se pide todo a la vez.

- [ ] **Step 4: Ejecutar los tests y ver que pasan**

Run: `powershell -ExecutionPolicy Bypass -File tools/test_core.ps1`
Expected: `OK: todas las pruebas pasan` y una línea con las cuatro demandas.

- [ ] **Step 5: Commit**

```bash
git add include/obd/profile.h src/core/profile.c tests/test_main.c
git commit -m "F3: reparto por pantalla; la demanda de cada pantalla cabe en el enlace"
```

---

### Task 4: Programar el planificador por pantalla (F3, capa PSP)

**Files:**
- Modify: `include/obd/scheduler.h`, `src/core/scheduler.c`, `src/psp/main.c`
- Test: `tests/test_scheduler.c`

**Interfaces:**
- Consumes: `obd_profile_period(uint8_t pid, int visible)` y `obd_profile_visible_mask(uint8_t pid)` de la Task 2.
- Produces:
  - `void obd_sched_set_period(obd_scheduler *sched, int idx, uint16_t period_ms, int due_now)` — cambia el periodo de un PID; con `due_now` distinto de 0 su `next_due_ms` pasa a `now` (que el llamante ya haya fijado).
  - En `src/psp/main.c`: `static void apply_screen_schedule(uint32_t now)` — recalcula el periodo de cada PID según la pantalla activa y deja pendientes los que acaban de aparecer.

- [ ] **Step 1: Escribir el test que falla**

En `tests/test_scheduler.c`, añadir:

```c
/* Cambiar el periodo de un PID en caliente: el planificador tiene que
 * seguir sirviendo y el PID pendiente de inmediato. */
static void test_set_period(void)
{
    obd_scheduler s;
    obd_sched_init(&s);
    obd_sched_add(&s, 0x0C, 200, 0);
    obd_sched_add(&s, 0x0B, 400, 0);

    /* Nada ha vencido todavia. */
    CHECK(obd_sched_next(&s, 0u) < 0);

    /* El PID pasa a estar pendiente de inmediato. */
    obd_sched_set_period(&s, 0, 800, 1);
    CHECK(obd_sched_next(&s, 100u) == 0);

    /* Con el periodo cambiado, el PID no vuelve a pedir hasta que
     * venza el nuevo periodo. */
    obd_sched_begin(&s, 0, 100u);
    obd_sched_end(&s, 0, 100u, 1);
    CHECK(obd_sched_next(&s, 500u) < 0);
    CHECK(obd_sched_next(&s, 900u) == 0);

    /* Reprogramar con una peticion EN VUELO no puede dejar el
     * planificador colgado: el PID en vuelo sigue en vuelo y, cuando
     * termina, el planificador sigue sirviendo. */
    obd_sched_begin(&s, 0, 1000u);
    obd_sched_set_period(&s, 0, 300, 1);
    CHECK(obd_sched_next(&s, 1000u) < 0);   /* hay una en vuelo */
    obd_sched_end(&s, 0, 1000u, 1);
    CHECK(obd_sched_next(&s, 1000u) >= 0);  /* sigue sirviendo */
}
```

- [ ] **Step 2: Ejecutar el test y ver que falla**

Run: `powershell -ExecutionPolicy Bypass -File tools/test_core.ps1`
Expected: FAIL por `obd_sched_set_period` no declarada.

- [ ] **Step 3: Implementar `obd_sched_set_period` en el núcleo**

En `include/obd/scheduler.h` declarar la función. En `src/core/scheduler.c`:

```c
void obd_sched_set_period(obd_scheduler *sched, int idx, uint16_t period_ms,
                          int due_now)
{
    if (idx < 0 || idx >= sched->count) {
        return;
    }
    sched->items[idx].period_ms = (period_ms > 0) ? period_ms : 100;
    if (due_now) {
        sched->items[idx].next_due_ms = 0;
    }
}
```

Con `next_due_ms = 0` el PID queda vencido (la comparación de `obd_sched_next` es `(int32_t)(now - item->next_due_ms) < 0` → con 0 y `now` actual es >= 0 → vencido).

- [ ] **Step 4: Ejecutar los tests y ver que pasan**

Run: `powershell -ExecutionPolicy Bypass -File tools/test_core.ps1`
Expected: `OK: todas las pruebas pasan`.

- [ ] **Step 5: Implementar el conjunto visible y la reprogramación en la capa PSP**

En `src/psp/main.c`:

1. Añadir el mapeo widget → PID (no existe hoy):

```c
/* PID de OBD que pinta cada elemento del panel. W_BOOST es derivado
 * (MAP + baro), asi que devuelve 0. */
static uint8_t widget_pid(int item)
{
    switch (item) {
    case W_RPM:     return 0x0C;
    case W_SPEED:   return 0x0D;
    case W_ECT:     return 0x05;
    case W_IAT:     return 0x0F;
    case W_MAP:     return 0x0B;
    case W_MAF:     return 0x10;
    case W_THR:     return (uint8_t)g_thr_pid;
    case W_LOAD:    return 0x04;
    case W_PLATE:   return 0x0D;
    case W_VOLT:    return 0x42;
    case W_OIL:     return 0x5C;
    case W_FUEL:    return 0x2F;
    case W_STFT:    return 0x06;
    case W_LTFT:    return 0x07;
    case W_TIMING:  return 0x0E;
    case W_O2_1:    return 0x14;
    case W_O2_2:    return 0x15;
    case W_DIST_MIL:return 0x21;
    case W_BARO:    return 0x33;
    default:        return 0;
    }
}
```

2. Añadir `static void apply_screen_schedule(uint32_t now)`:

```c
/* Recalcula el periodo de cada PID segun la pantalla activa. Lo que se
 * esta mirando va a su periodo base; lo demas, a un periodo lento. Los
 * PID que acaban de aparecer pasan a estar pendientes de inmediato. */
static void apply_screen_schedule(uint32_t now)
{
    uint8_t mode_mask;

    switch (g_mode) {
    case MODE_ENGINE:
        mode_mask = 0x02;
        break;
    case MODE_MOTOR:
        mode_mask = 0x04;
        break;
    case MODE_TELEMETRY:
        mode_mask = 0x08;
        break;
    default:
        mode_mask = 0x01;
        break;
    }

    /* Los huecos del panel y las series de telemetria pueden llevar
     * cualquier dato: si esta a la vista, va a su periodo base. */
    uint64_t forced = 0;
    if (g_mode == MODE_QV) {
        for (int i = 0; i < DASH_SLOTS; i++) {
            uint8_t pid = widget_pid(g_slot_item[i]);
            if (pid != 0) {
                forced |= (uint64_t)1u << pid;
            }
        }
    } else if (g_mode == MODE_TELEMETRY) {
        for (int i = 0; i < TEL_SLOTS; i++) {
            uint8_t pid = widget_pid(g_tel_item[i]);
            if (pid != 0) {
                forced |= (uint64_t)1u << pid;
            }
        }
    }

    for (int i = 0; i < g_sched.count; i++) {
        uint8_t pid = g_sched.items[i].pid;
        uint8_t mask = obd_profile_visible_mask(pid);
        int visible = (mask & mode_mask) != 0 ||
                      (forced & ((uint64_t)1u << pid)) != 0;
        uint16_t period = obd_profile_period(pid, visible);
        int was_due = (int32_t)(now - g_sched.items[i].next_due_ms) >= 0;
        obd_sched_set_period(&g_sched, i, period,
                             visible && !was_due ? 1 : 0);
    }
}
```

**Nota sobre el tipo de `forced`**: el PID más alto de la tabla es `0x33`
(51), así que `1u << pid` con 32 bits sería indefinido. `forced` es
`uint64_t` y el desplazamiento es `(uint64_t)1u << pid`.

3. Llamar a `apply_screen_schedule(now)`:
   - al final de `build_scheduler()` (con `now_ms()`),
   - cada vez que cambie `g_mode` (en `handle_mode_input`, que ya devuelve `changed`).

En `handle_mode_input`, tras cambiar `g_mode`, llamar `apply_screen_schedule(now_ms())`.

- [ ] **Step 6: Compilar y comprobar que no hay avisos**

Run: `powershell -ExecutionPolicy Bypass -File tools/build.ps1`
Expected: sin `warning` ni `error` en la salida.

- [ ] **Step 7: Commit**

```bash
git add include/obd/scheduler.h src/core/scheduler.c src/psp/main.c tests/test_scheduler.c
git commit -m "F3: el planificador se reprograma segun la pantalla activa"
```

---

### Task 5: Hueco mínimo del adaptador, ajustable (F1)

**Files:**
- Modify: `src/psp/main.c` (constante, enforcement, ajuste del menú), `include/obd/profile.h` o un头 del propio main.c para el valor por defecto.

**Interfaces:**
- Consumes: nada de tareas anteriores.
- Produces:
  - `static int g_elm_min_gap_ms` — hueco mínimo entre el fin de una respuesta y el envío del siguiente comando. Por defecto **8**.
  - `static uint32_t g_last_resp_ms` — instante en que se completó la última respuesta (anclaje del hueco).
  - El ajuste de menú `HUECO ELM` con valores 0/2/4/6/8/12/17 ms, guardado en `settings.txt` como `elm_gap=`.

- [ ] **Step 1: Añadir el hueco al estado de la aplicación**

En `src/psp/main.c`, junto al resto de ajustes persistidos:

```c
#define ELM_GAP_DEFAULT_MS 8
static const int g_elm_gap_values[7] = { 0, 2, 4, 6, 8, 12, 17 };
```

- [ ] **Step 2: Enforcement no bloqueante en el bucle**

En `run_dashboard`, tras procesar una respuesta, se guarda `g_last_resp_ms = now_ms()`. En el bloque de sondeo (donde hoy se decide enviar), la condición de envío pasa a ser:

```c
/* FASE F1: el ELM327 necesita un hueco minimo entre comandos. Con hueco
 * 0 retrasa la respuesta un tick de 16,7 ms y la capacidad cae de 12 a
 * 9,25 peticiones/s. */
int gap_ok = (g_elm_min_gap_ms <= 0) ||
             ((int32_t)(now_ms() - g_last_resp_ms) >= g_elm_min_gap_ms);
```

Si `gap_ok` es falso, **no se envía** y el bucle sigue (duerme 2 ms en la
cadencia, no espera al VBlank). El acumulador `gap_acc` se mueve a esta ruta
de envío diferida, de forma que la métrica `gap=` mida el hueco **realmente
aplicado**.

Además, la métrica del log pasa a tener dos campos:

```
gap=<medido> gapcfg=<configurado>
```

- [ ] **Step 3: Añadir el ajuste al menú**

- `SET_COUNT` pasa de 16 a **17** y `SET_EXIT` de 15 a 16.
- La rejilla del overlay pasa de 2×8 a **2×9** (`per_col` de 8 a 9) para que
  quepan las 17 entradas.
- El nuevo elemento va justo después de `MUESTREO` (índice 5), así que los
  índices de las entradas posteriores se desplazan **+1**. Revisar el bloque
  `draw_settings_overlay` (la cadena de `if (i == N)`) y el handler
  `handle_settings_input`.
- Nombre: `tr("HUECO ELM")` con la entrada EN correspondiente en la tabla
  `tr()`.
- El valor se muestra como `"%d ms"`.

- [ ] **Step 4: Registrar el cambio en el log**

Al cambiar el ajuste, escribir en el log:

```c
log_file_printf("AJUSTE: hueco ELM = %d ms\n", g_elm_min_gap_ms);
log_sync();
```

Esto permite segmentar un viaje de calibración en un solo fichero.

- [ ] **Step 5: Guardar y cargar**

En `settings_save()` escribir `elm_gap=%d` y en `settings_load()` leerlo,
validando que el valor esté en la lista `g_elm_gap_values` (si no, usar
`ELM_GAP_DEFAULT_MS`).

- [ ] **Step 6: Compilar y comprobar**

Run: `powershell -ExecutionPolicy Bypass -File tools/build.ps1`
Expected: sin avisos. Luego `powershell -ExecutionPolicy Bypass -File tools/test_core.ps1` → `OK`.

- [ ] **Step 7: Commit**

```bash
git add src/psp/main.c
git commit -m "F1: hueco minimo entre comandos al ELM327, ajustable desde el menu"
```

---

### Task 6: Documentación y build

**Files:**
- Modify: `RUN.txt`, `README.md`, `README_EN.md`, `docs/ROADMAP.md`, `docs/ARQUITECTURA.md`
- Modify: `src/psp/main.c` (`BUILD_TAG`)

**Interfaces:**
- Consumes: todo lo anterior.
- Produces: `BUILD_TAG` = `"fase2.43 2026-11-16 FASE F1+F3+F2.5 hueco ajustable, reparto por pantalla y edad global"`.

- [ ] **Step 1: Actualizar `BUILD_TAG`**

En `src/psp/main.c`, el valor exacto de arriba.

- [ ] **Step 2: Documentar el ajuste nuevo en `RUN.txt`**

En la sección "AJUSTES DE TEMA", añadir la línea:

```
    HUECO ELM   hueco minimo entre comandos al adaptador (0-17 ms).
                8 ms es el valor recomendado; 0 hace que el ELM327
                retrase la respuesta y baja el caudal de datos.
```

- [ ] **Step 3: Documentar en los README (ES/EN) y en `docs/ARQUITECTURA.md`**

En la sección de arquitectura, añadir la subsection del reparto por pantalla y la del hueco mínimo, con los valores de la tabla de la Task 2 y la evidencia de `phase1_071`/`phase1_072`.

- [ ] **Step 4: Actualizar `docs/ROADMAP.md`**

Marcar la Fase 2.7 con lo implementado y añadir la Fase 2.8 (F1+F3).

- [ ] **Step 5: Build, tests, instalar y commit**

```bash
powershell -ExecutionPolicy Bypass -File tools/build.ps1
powershell -ExecutionPolicy Bypass -File tools/test_core.ps1
cp dist/PSPTorqueOBD/EBOOT.PBP "E:/PSP/GAME/PSPTorqueOBD/EBOOT.PBP"
git add -A
git commit -m "FASE F1+F3+F2.5: documentacion y build"
```

- [ ] **Step 6: Registrar el SHA-256 y el hash de la release**

Anotar el SHA-256 del `EBOOT.PBP` instalado y crear la release `v2.43` en GitHub
con el binario como asset (solo tras validar en hardware).

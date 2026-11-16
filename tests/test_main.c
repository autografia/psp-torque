#include "obd/profile.h"
#include "obd/samples.h"
#include "test_util.h"

#include <stdio.h>

void test_parser(void);
void test_pid(void);
void test_boost(void);
void test_scheduler(void);
void test_samples(void);
void test_discovery(void);
void test_smooth(void);

int g_failures = 0;

static void test_profile(void)
{
    CHECK(PROFILE_QV_POLL_COUNT >= 5);
    CHECK(PROFILE_ENGINE_POLL_COUNT >= 10);
    CHECK(PROFILE_QV_POLL[0].pid == 0x0C);
    CHECK(PROFILE_QV_POLL[1].pid == 0x0B);
    CHECK(PROFILE_QV_POLL[0].period_ms == 250);
    CHECK(PROFILE_QV_POLL[1].period_ms == 500);
    CHECK(PROFILE_BARO_FALLBACK_KPA > 50.0f);
    CHECK(PROFILE_BARO_FALLBACK_KPA < 110.0f);
}

/* FASE E2: el presupuesto de peticiones del planificador.
 *
 * El enlace ELM327 WiFi da ~18 peticiones/s (unos 55 ms por peticion
 * completa: envio, respuesta del coche y hueco entre peticiones). Si la
 * suma de 1000/periodo de todos los PID que la app puede llegar a
 * sondear supera ese techo, el planificador se satura y TODOS los PID
 * se refrescan mas tarde de lo pedido: el RPM pasaria de 350 ms a
 * 700-800 ms y el tacometro iria con retraso.
 *
 * Este test calcula la demanda en el caso peor (todos los PID opcionales
 * presentes a la vez) y la compara con el presupuesto. Es la regresion
 * que habria que volver a tocar si alguien baja los periodos. */
static void test_link_budget(void)
{
    float demand = 0.0f;

    for (int i = 0; i < PROFILE_QV_POLL_COUNT; i++) {
        demand += 1000.0f / (float)PROFILE_QV_POLL[i].period_ms;
    }
    for (int i = 0; i < PROFILE_EXTRA_POLL_COUNT; i++) {
        demand += 1000.0f / (float)PROFILE_EXTRA_POLL[i].period_ms;
    }
    for (int i = 0; i < PROFILE_MOTOR_POLL_COUNT; i++) {
        demand += 1000.0f / (float)PROFILE_MOTOR_POLL[i].period_ms;
    }

    /* PID que solo se sondean si la mascara los soporta. */
    demand += 1000.0f / (float)PROFILE_PID_MAF_PERIOD_MS;
    demand += 1000.0f / (float)PROFILE_PID_PEDAL_PERIOD_MS;
    demand += 1000.0f / (float)PROFILE_PID_VOLT_PERIOD_MS;
    demand += 1000.0f / (float)PROFILE_PID_OIL_PERIOD_MS;
    demand += 1000.0f / (float)PROFILE_PID_FUEL_PERIOD_MS;

    printf("  demanda del planificador: %.2f req/s (presupuesto %.1f)\n",
           demand, (double)PROFILE_LINK_BUDGET_REQ_S);

    CHECK(demand <= PROFILE_LINK_BUDGET_REQ_S);

    /* Y al reves: si la demanda cae mucho, se esta desaprovechando el
     * enlace y el panel se queda mas lento de lo necesario. */
    CHECK(demand >= PROFILE_LINK_BUDGET_REQ_S * 0.60f);

    /* El RPM y el MAP son los que van en el tacometro: su periodo debe
     * ser corto comparados con el resto. */
    CHECK(PROFILE_QV_POLL[0].period_ms <= 300);
    CHECK(PROFILE_QV_POLL[1].period_ms <= 600);

    /* Ningun PID por debajo de 200 ms: por debajo de eso la demanda se
     * dispara y el enlace no da mas. */
    for (int i = 0; i < PROFILE_QV_POLL_COUNT; i++) {
        CHECK(PROFILE_QV_POLL[i].period_ms >= 200);
    }
    for (int i = 0; i < PROFILE_EXTRA_POLL_COUNT; i++) {
        CHECK(PROFILE_EXTRA_POLL[i].period_ms >= 200);
    }
    for (int i = 0; i < PROFILE_MOTOR_POLL_COUNT; i++) {
        CHECK(PROFILE_MOTOR_POLL[i].period_ms >= 1000);
    }
}

/* La edad global del panel es la de la muestra mas reciente de
 * CUALQUIER PID: si el RPM se retrasa pero el resto fluye, el panel
 * esta vivo y no debe avisar de datos viejos. */
static void test_data_age(void)
{
    obd_sample_store st;
    obd_store_init(&st);

    /* Sin muestras: edad desconocida. */
    CHECK(obd_store_newest_ts(&st) == 0u);

    obd_store_set(&st, 0x0C, 1000.0f, OBD_UNIT_RPM, 1000u, OBD_SRC_REAL);
    obd_store_set(&st, 0x0B, 30.0f, OBD_UNIT_KPA, 1500u, OBD_SRC_REAL);

    /* La mas reciente manda, no la del RPM. */
    CHECK(obd_store_newest_ts(&st) == 1500u);

    /* Una muestra invalidada no cuenta. */
    obd_store_invalidate(&st, 0x0B, 1600u);
    CHECK(obd_store_newest_ts(&st) == 1000u);
}

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

int main(void)
{
    test_parser();
    test_pid();
    test_boost();
    test_scheduler();
    test_samples();
    test_discovery();
    test_profile();
    test_profile_periods();
    test_data_age();
    test_link_budget();
    test_smooth();

    if (g_failures != 0) {
        printf("%d comprobaciones fallidas\n", g_failures);
        return 1;
    }

    printf("OK: todas las pruebas pasan\n");
    return 0;
}

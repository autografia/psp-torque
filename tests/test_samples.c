#include "obd/samples.h"
#include "test_util.h"

void test_samples(void)
{
    obd_sample_store store;
    obd_store_init(&store);
    CHECK(store.count == 0);
    CHECK(obd_store_get(&store, 0x0C) == 0);

    obd_store_set(&store, 0x0C, 1726.0f, OBD_UNIT_RPM, 1000,
                  OBD_SRC_REAL);
    CHECK(store.count == 1);

    const obd_sample *rpm = obd_store_get(&store, 0x0C);
    CHECK(rpm != 0);
    CHECK(rpm->valid == 1);
    CHECK(rpm->source == OBD_SRC_REAL);
    CHECK_NEAR(rpm->value, 1726.0f, 0.01);
    CHECK(rpm->ts_ms == 1000);

    CHECK(obd_sample_stale(rpm, 1400, 500) == 0);
    CHECK(obd_sample_stale(rpm, 1600, 500) == 1);

    /* Actualizar no duplica. */
    obd_store_set(&store, 0x0C, 1800.0f, OBD_UNIT_RPM, 2000,
                  OBD_SRC_REAL);
    CHECK(store.count == 1);
    rpm = obd_store_get(&store, 0x0C);
    CHECK_NEAR(rpm->value, 1800.0f, 0.01);
    CHECK(rpm->ts_ms == 2000);

    /* Invalidar. */
    obd_store_invalidate(&store, 0x0C, 3000);
    rpm = obd_store_get(&store, 0x0C);
    CHECK(rpm->valid == 0);
    CHECK(rpm->source == OBD_SRC_UNAVAILABLE);
    CHECK(obd_sample_stale(rpm, 3100, 500) == 1);

    /* Invalidate de un PID inexistente no cambia nada. */
    obd_store_invalidate(&store, 0x5C, 4000);
    CHECK(store.count == 1);

    /* Limite del almacen. */
    for (int i = 0; i < 40; i++) {
        obd_store_set(&store, (uint8_t)(0x40 + i), (float)i,
                      OBD_UNIT_NONE, 5000, OBD_SRC_REAL);
    }
    CHECK(store.count == OBD_SAMPLE_MAX);
    CHECK(obd_store_get(&store, 0x0C) != 0);
}

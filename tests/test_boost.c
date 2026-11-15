#include "obd/boost.h"
#include "test_util.h"

void test_boost(void)
{
    boost_state state;

    /* Fallback: sin MAP no hay boost, pero el baro esta listo. */
    boost_init(&state, 101.3f);
    CHECK(state.baro_valid == 1);
    CHECK(state.baro_source == BARO_CONFIG);
    CHECK(state.boost_valid == 0);

    /* MAP real del MiTo con contacto: 97 kPa. */
    boost_set_map(&state, 97.0f, 1000);
    CHECK(state.map_valid == 1);
    CHECK(state.boost_valid == 1);
    CHECK_NEAR(state.boost_bar, -0.043f, 0.001);
    CHECK(state.boost_is_estimate == 1);

    /* Llega el PID 0x33: manda sobre el fallback. */
    boost_set_baro_pid(&state, 98.0f, 2000);
    CHECK(state.baro_source == BARO_PID);
    CHECK(state.boost_valid == 1);
    CHECK_NEAR(state.boost_bar, -0.01f, 0.001);
    CHECK(state.boost_is_estimate == 0);

    /* Bajo carga. */
    boost_set_map(&state, 180.0f, 3000);
    CHECK_NEAR(state.boost_bar, 0.82f, 0.001);
    CHECK(state.boost_is_estimate == 0);

    /* Baro no plausible: se ignora. */
    boost_set_baro_pid(&state, 20.0f, 4000);
    CHECK(state.baro_source == BARO_PID);
    CHECK_NEAR(state.baro_kpa, 98.0f, 0.001);

    /* Sin MAP no se inventa boost. */
    boost_state no_map;
    boost_init(&no_map, 101.3f);
    CHECK(no_map.boost_valid == 0);
    CHECK(no_map.map_valid == 0);

    /* Captura key-on valida. */
    boost_state keyon;
    boost_init(&keyon, 101.3f);
    CHECK(boost_capture_key_on(&keyon, 97.0f, 1, 0.0f, 1, 0.0f,
                               5000, 5000) == 1);
    CHECK(keyon.baro_source == BARO_KEY_ON);
    CHECK_NEAR(keyon.baro_kpa, 97.0f, 0.001);
    CHECK(keyon.boost_is_estimate == 0);

    /* No se vuelve a capturar ni la sobrescribe el key-on. */
    CHECK(boost_capture_key_on(&keyon, 90.0f, 1, 0.0f, 1, 0.0f,
                               6000, 6000) == 0);
    CHECK_NEAR(keyon.baro_kpa, 97.0f, 0.001);

    /* Con el motor en marcha no se captura. */
    boost_state running;
    boost_init(&running, 101.3f);
    CHECK(boost_capture_key_on(&running, 40.0f, 1, 800.0f, 1, 0.0f,
                               1000, 1000) == 0);
    CHECK(running.baro_source == BARO_CONFIG);

    /* Fuera de la ventana de inicio. */
    CHECK(boost_capture_key_on(&running, 97.0f, 1, 0.0f, 1, 0.0f,
                               1000, 70000) == 0);

    /* MAP no plausible. */
    CHECK(boost_capture_key_on(&running, 120.0f, 1, 0.0f, 1, 0.0f,
                               1000, 1000) == 0);

    /* RPM desconocido: no se puede asegurar que el motor este parado. */
    CHECK(boost_capture_key_on(&running, 97.0f, 0, 0.0f, 1, 0.0f,
                               1000, 1000) == 0);

    /* En movimiento no se captura. */
    CHECK(boost_capture_key_on(&running, 97.0f, 1, 0.0f, 1, 30.0f,
                               1000, 1000) == 0);

    /* El PID 0x33 tiene prioridad sobre una captura posterior. */
    boost_state prio;
    boost_init(&prio, 101.3f);
    boost_set_baro_pid(&prio, 99.0f, 1000);
    CHECK(boost_capture_key_on(&prio, 97.0f, 1, 0.0f, 1, 0.0f,
                               2000, 2000) == 0);
    CHECK(prio.baro_source == BARO_PID);
    CHECK_NEAR(prio.baro_kpa, 99.0f, 0.001);

    /* El fallback invalido se corrige. */
    boost_state bad;
    boost_init(&bad, 10.0f);
    CHECK_NEAR(bad.baro_kpa, 101.3f, 0.001);
}

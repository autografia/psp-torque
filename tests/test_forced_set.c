#include "obd/profile.h"
#include "test_util.h"

#include <string.h>

/* FASE F3: los huecos configurables del panel pueden contener cualquier
 * dato, incluidos PID altos como 0x5C (92) o 0x49 (73). Una mascara de
 * bits con `1u << pid` daria comportamiento indefinido para esos PID;
 * el conjunto debe funcionar para todo el rango de OBD-II. */
void test_forced_set(void)
{
    obd_forced_set set;
    memset(&set, 0, sizeof(set));

    CHECK(!obd_forced_set_has(&set, 0x0C));
    CHECK(!obd_forced_set_has(&set, 0x5C));

    obd_forced_set_add(&set, 0x5C);   /* 92: no cabe en 64 bits */
    CHECK(obd_forced_set_has(&set, 0x5C));
    CHECK(!obd_forced_set_has(&set, 0x5A));

    obd_forced_set_add(&set, 0x49);   /* 73: no cabe en 64 bits */
    obd_forced_set_add(&set, 0x0C);
    CHECK(obd_forced_set_has(&set, 0x49));
    CHECK(obd_forced_set_has(&set, 0x0C));
    CHECK(!obd_forced_set_has(&set, 0x42));
}

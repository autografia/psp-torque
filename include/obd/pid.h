#ifndef OBD_PID_H
#define OBD_PID_H

#include <stdint.h>

/* Unidades de trabajo del nucleo. La UI decide como mostrarlas. */
typedef enum {
    OBD_UNIT_NONE = 0,
    OBD_UNIT_RPM,
    OBD_UNIT_KPH,
    OBD_UNIT_CELSIUS,
    OBD_UNIT_KPA,
    OBD_UNIT_PERCENT,
    OBD_UNIT_VOLT,
    OBD_UNIT_DEGREES,
    OBD_UNIT_GRAMS_SEC,
    OBD_UNIT_LPH,
    OBD_UNIT_KM,
    OBD_UNIT_SECONDS
} obd_unit;

typedef struct {
    float value;
    obd_unit unit;
    int valid;
} obd_pid_value;

/* Bytes de datos esperados para un PID (0 = desconocido). */
int obd_pid_expected_bytes(uint8_t pid);

/* Decodifica los bytes de datos (sin el 0x41 ni el PID).
 * Devuelve 0 si el PID es conocido y la longitud correcta, -1 si no. */
int obd_pid_decode(uint8_t pid, const uint8_t *data, int len,
                   obd_pid_value *out);

/* Algunas ECU (MiTo 1.4 170cv) codifican los trims como byte con signo
 * y el avance sin el offset de 64 grados. Se activa solo al detectar
 * sus mascaras reales; el resto de coches mantiene la formula OBD. */
void obd_pid_set_mito_encoding(int enabled);

const char *obd_unit_symbol(obd_unit unit);

/* Mascaras 0100/0120/...: 4 bytes big-endian, bit 7 = primer PID. */
uint32_t obd_pid_mask(const uint8_t *data, int len);

/* pid en 0x01..0x20 dentro del bloque de la mascara. */
int obd_mask_supports(uint32_t mask, uint8_t pid);

#endif

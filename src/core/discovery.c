#include "obd/discovery.h"

#include <ctype.h>
#include <string.h>

static int hex_value(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return -1;
}

static int is_hex_token(const char *text, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (hex_value(text[i]) < 0) {
            return 0;
        }
    }
    return len > 0;
}

static const char *skip_spaces(const char *p)
{
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    return p;
}

static size_t token_len(const char *p)
{
    size_t n = 0;
    while (p[n] != '\0' && p[n] != ' ' && p[n] != '\t') {
        n++;
    }
    return n;
}

/*
 * Cabecera CAN de 29 bits escrita con espacios: "18 DA F1 10".
 * Solo se interpreta asi si empieza por 18, que es el prefijo habitual
 * de las respuestas de diagnostico UDS en CAN de 29 bits.
 */
static const char *parse_header_29(const char *p, char *out, size_t out_size)
{
    char hdr[9];
    int used = 0;

    if (strncmp(p, "18", 2) != 0) {
        return NULL;
    }

    for (int i = 0; i < 4; i++) {
        p = skip_spaces(p);
        size_t n = token_len(p);
        if (n != 2 || !is_hex_token(p, 2)) {
            return NULL;
        }
        if (used + 2 >= (int)sizeof(hdr)) {
            return NULL;
        }
        hdr[used++] = p[0];
        hdr[used++] = p[1];
        p += n;
    }
    hdr[used] = '\0';

    if (out_size < (size_t)used + 1) {
        return NULL;
    }
    memcpy(out, hdr, (size_t)used + 1);
    return p;
}

int obd_disc_parse_line(const char *text, obd_disc_line *out)
{
    if (text == NULL || out == NULL) {
        return 0;
    }

    memset(out, 0, sizeof(*out));

    const char *p = skip_spaces(text);
    if (*p == '\0') {
        return 0;
    }

    /* Prefijo "0:", "1:", ... de algunos ELM327 en mensajes largos. */
    if (p[0] != '\0' && p[1] == ':' && hex_value(p[0]) >= 0) {
        p += 2;
        p = skip_spaces(p);
    } else {
        size_t n = token_len(p);

        if ((n == 3 || n == 8) && is_hex_token(p, n)) {
            if (n >= sizeof(out->header)) {
                return 0;
            }
            memcpy(out->header, p, n);
            out->header[n] = '\0';
            p += n;
        } else if (n == 2 && is_hex_token(p, n)) {
            const char *after = parse_header_29(p, out->header,
                                                 sizeof(out->header));
            if (after != NULL) {
                p = after;
            }
            /* Si no era una cabecera de 29 bits, se deja como datos. */
        }
    }

    int count = 0;
    p = skip_spaces(p);
    while (*p != '\0') {
        if (hex_value(p[0]) < 0 || hex_value(p[1]) < 0) {
            break;
        }
        if (count < OBD_DISC_MAX_DATA) {
            int hi = hex_value(p[0]);
            int lo = hex_value(p[1]);
            out->data[count++] = (uint8_t)((hi << 4) | lo);
        }
        p += 2;
        p = skip_spaces(p);
    }

    out->data_len = count;
    return count > 0;
}

int obd_disc_payload(const obd_disc_line *line, const uint8_t **payload)
{
    if (line == NULL || payload == NULL || line->data_len <= 0) {
        return 0;
    }

    const uint8_t *d = line->data;
    int n = line->data_len;
    int type = (d[0] >> 4) & 0x0F;

    if (type == 0) {
        int len = d[0] & 0x0F;
        if (len <= 0 || n < 2) {
            return 0;
        }
        if (len > n - 1) {
            len = n - 1;
        }
        *payload = d + 1;
        return len;
    }

    if (type == 1) {
        if (n < 3) {
            return 0;
        }
        int len = ((d[0] & 0x0F) << 8) | d[1];
        if (len > n - 2) {
            len = n - 2;
        }
        *payload = d + 2;
        return len;
    }

    if (type == 2) {
        *payload = d + 1;
        return n - 1;
    }

    /* Flow control (0x3N): no es datos de respuesta. */
    if (type == 3) {
        return 0;
    }

    /* Sin PCI: la trama empieza directamente por el servicio (0x41, 0x62...). */
    *payload = d;
    return n;
}

void obd_disc_ascii(const uint8_t *data, int len, char *out, size_t out_size)
{
    if (out == NULL || out_size == 0) {
        return;
    }

    size_t used = 0;
    int started = 0;

    for (int i = 0; i < len && used + 1 < out_size; i++) {
        uint8_t c = data[i];
        if (c >= 0x20 && c <= 0x7E) {
            out[used++] = (char)c;
            started = 1;
        } else if (started) {
            break;
        }
    }
    out[used] = '\0';

    while (used > 0 && out[used - 1] == ' ') {
        out[--used] = '\0';
    }
}

uint32_t obd_disc_mask(const uint8_t *data, int len)
{
    if (data == NULL || len < 4) {
        return 0;
    }
    return ((uint32_t)data[0] << 24) |
           ((uint32_t)data[1] << 16) |
           ((uint32_t)data[2] << 8) |
           (uint32_t)data[3];
}

#include "obd/elm327.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static void trim_upper(char *dst, size_t dst_size, const char *src)
{
    while (*src == ' ' || *src == '\t' || *src == '\r' || *src == '\n') {
        src++;
    }

    size_t len = strlen(src);
    while (len > 0) {
        char c = src[len - 1];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            len--;
        } else {
            break;
        }
    }

    if (len >= dst_size) {
        len = dst_size - 1;
    }
    for (size_t i = 0; i < len; i++) {
        dst[i] = (char)toupper((unsigned char)src[i]);
    }
    dst[len] = '\0';
}

static int hex_nibble(char c, int *value)
{
    if (c >= '0' && c <= '9') {
        *value = c - '0';
        return 1;
    }
    if (c >= 'A' && c <= 'F') {
        *value = c - 'A' + 10;
        return 1;
    }
    if (c >= 'a' && c <= 'f') {
        *value = c - 'a' + 10;
        return 1;
    }
    return 0;
}

static int parse_hex(const char *text, uint8_t *out, int max)
{
    int count = 0;

    while (*text != '\0') {
        if (*text == ' ' || *text == '\t') {
            text++;
            continue;
        }

        int hi = 0;
        int lo = 0;
        if (!hex_nibble(text[0], &hi) || !hex_nibble(text[1], &lo)) {
            return -1;
        }
        if (count >= max) {
            return -1;
        }
        out[count++] = (uint8_t)((hi << 4) | lo);
        text += 2;
    }

    return count;
}

static int starts_with(const char *text, const char *prefix)
{
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

elm_parse_result elm_parse_line(const char *line, elm_frame *out)
{
    if (out == NULL) {
        return ELM_PARSE_NONE;
    }
    memset(out, 0, sizeof(*out));

    char text[ELM_MAX_RAW];
    trim_upper(text, sizeof(text), line ? line : "");

    if (text[0] == '\0') {
        return ELM_PARSE_NONE;
    }

    if (strcmp(text, "NO DATA") == 0) {
        return ELM_PARSE_NO_DATA;
    }
    if (strcmp(text, "UNABLE TO CONNECT") == 0) {
        return ELM_PARSE_UNABLE;
    }
    if (starts_with(text, "SEARCHING") ||
        starts_with(text, "BUS INIT") ||
        starts_with(text, "STOPPED")) {
        return ELM_PARSE_NONE;
    }
    if (strcmp(text, "?") == 0 ||
        starts_with(text, "ERROR") ||
        starts_with(text, "CAN ERROR") ||
        starts_with(text, "BUS ERROR") ||
        starts_with(text, "BUFFER FULL") ||
        starts_with(text, "DATA ERROR") ||
        starts_with(text, "RX ERROR") ||
        starts_with(text, "FB ERROR")) {
        return ELM_PARSE_ERROR;
    }
    if (strcmp(text, "OK") == 0 ||
        starts_with(text, "ELM327") ||
        starts_with(text, "AT ") ||
        starts_with(text, "AT@")) {
        return ELM_PARSE_NONE;
    }

    /* Tramas ISO-TP multilinea ("0:", "1:", ...): no soportadas en modo 01. */
    if (text[1] == ':') {
        return ELM_PARSE_NONE;
    }

    uint8_t bytes[ELM_MAX_DATA + 2];
    int count = parse_hex(text, bytes, (int)sizeof(bytes));
    if (count < 3) {
        return ELM_PARSE_NONE;
    }

    /* Respuesta de datos: 0x41 (modo 01), 0x49 (modo 09)... */
    if (bytes[0] < 0x41 || bytes[0] > 0x4A) {
        return ELM_PARSE_NONE;
    }

    out->mode = bytes[0];
    out->pid = bytes[1];
    out->data_len = count - 2;
    for (int i = 0; i < out->data_len; i++) {
        out->data[i] = bytes[i + 2];
    }

    size_t raw_len = strlen(text);
    if (raw_len >= sizeof(out->raw)) {
        raw_len = sizeof(out->raw) - 1;
    }
    memcpy(out->raw, text, raw_len);
    out->raw[raw_len] = '\0';

    return ELM_PARSE_DATA;
}

int elm_frame_matches(const elm_frame *frame, uint8_t mode, uint8_t pid)
{
    if (frame == NULL) {
        return 0;
    }
    return frame->mode == (uint8_t)(mode + 0x40) && frame->pid == pid;
}

int elm_build_request(char *buf, size_t size, uint8_t mode, uint8_t pid)
{
    if (buf == NULL || size < 5) {
        return -1;
    }
    int n = snprintf(buf, size, "%02X%02X", (unsigned)mode, (unsigned)pid);
    return (n == 4) ? 0 : -1;
}

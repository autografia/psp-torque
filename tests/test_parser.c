#include "obd/elm327.h"
#include "test_util.h"

#include <string.h>

void test_parser(void)
{
    elm_frame frame;
    elm_parse_result result;
    char cmd[8];

    /* Respuesta CAN real del MiTo: 010C con motor parado. */
    result = elm_parse_line("410C0000", &frame);
    CHECK(result == ELM_PARSE_DATA);
    CHECK(frame.mode == 0x41);
    CHECK(frame.pid == 0x0C);
    CHECK(frame.data_len == 2);
    CHECK(frame.data[0] == 0x00);
    CHECK(frame.data[1] == 0x00);
    CHECK(strcmp(frame.raw, "410C0000") == 0);

    /* Espacios y minusculas. */
    result = elm_parse_line("41 0c 1a f8\r\n", &frame);
    CHECK(result == ELM_PARSE_DATA);
    CHECK(frame.pid == 0x0C);
    CHECK(frame.data[0] == 0x1A);
    CHECK(frame.data[1] == 0xF8);

    /* Mascara de PIDs. */
    result = elm_parse_line("4100BE3EB811", &frame);
    CHECK(result == ELM_PARSE_DATA);
    CHECK(frame.pid == 0x00);
    CHECK(frame.data_len == 4);
    CHECK(frame.data[0] == 0xBE);
    CHECK(frame.data[3] == 0x11);

    /* Ruido e informacion. */
    CHECK(elm_parse_line("", &frame) == ELM_PARSE_NONE);
    CHECK(elm_parse_line("SEARCHING...", &frame) == ELM_PARSE_NONE);
    CHECK(elm_parse_line("BUS INIT: OK", &frame) == ELM_PARSE_NONE);
    CHECK(elm_parse_line("OK", &frame) == ELM_PARSE_NONE);
    CHECK(elm_parse_line("ELM327 v1.5", &frame) == ELM_PARSE_NONE);
    CHECK(elm_parse_line("010C", &frame) == ELM_PARSE_NONE);
    CHECK(elm_parse_line("7E8 04 41 0C 1A F8", &frame) == ELM_PARSE_NONE);
    CHECK(elm_parse_line("0: 49 02 01 00 00 00", &frame) == ELM_PARSE_NONE);

    /* Errores. */
    CHECK(elm_parse_line("NO DATA", &frame) == ELM_PARSE_NO_DATA);
    CHECK(elm_parse_line("UNABLE TO CONNECT", &frame) == ELM_PARSE_UNABLE);
    CHECK(elm_parse_line("?", &frame) == ELM_PARSE_ERROR);
    CHECK(elm_parse_line("CAN ERROR", &frame) == ELM_PARSE_ERROR);
    CHECK(elm_parse_line("BUS ERROR", &frame) == ELM_PARSE_ERROR);
    CHECK(elm_parse_line("STOPPED", &frame) == ELM_PARSE_NONE);

    /* Peticion y coincidencia. */
    CHECK(elm_build_request(cmd, sizeof(cmd), 0x01, 0x0C) == 0);
    CHECK(strcmp(cmd, "010C") == 0);
    CHECK(elm_build_request(cmd, sizeof(cmd), 0x01, 0x00) == 0);
    CHECK(strcmp(cmd, "0100") == 0);
    CHECK(elm_build_request(cmd, 4, 0x01, 0x0C) == -1);

    result = elm_parse_line("410C1AF8", &frame);
    CHECK(result == ELM_PARSE_DATA);
    CHECK(elm_frame_matches(&frame, 0x01, 0x0C) == 1);
    CHECK(elm_frame_matches(&frame, 0x01, 0x0D) == 0);

    result = elm_parse_line("490201", &frame);
    CHECK(result == ELM_PARSE_DATA);
    CHECK(elm_frame_matches(&frame, 0x09, 0x02) == 1);
}

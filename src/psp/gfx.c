/*
 * gfx.c - render 2D con GU para PSP Torque (Fase 2).
 *
 * Framebuffer 480x272 RGBA8888 con doble buffer en VRAM:
 *   draw 1: 0x000000
 *   draw 2: 0x088000
 * (512 * 272 * 4 = 0x88000 bytes por buffer)
 *
 * Se acumulan todos los vertices de un frame y se envian en un unico
 * sceGuDrawArray al final. Asi el buffer de vertices sigue siendo valido
 * hasta que la GE termina el frame (sceGuSync).
 *
 * IMPORTANTE: la GE lee los vertices directamente de memoria fisica, sin
 * pasar por la cache de datos de la CPU. Antes de dibujar hay que volcar
 * el rango con sceKernelDcacheWritebackRange, o la GE ve datos viejos y
 * la pantalla queda en negro.
 */

#include <pspkernel.h>
#include <pspgu.h>
#include <pspdisplay.h>
#include <math.h>
#include <string.h>

#include "gfx.h"
#include "font8x8.h"

/* Presets de color. QV_RED es el acento principal y QV_GREEN el
 * secundario; QV_CRIT se mantiene rojo para no perder los avisos. */
static const gfx_theme g_theme_presets[GFX_THEME_COUNT] = {
    /* 0 - QV Rosso (paleta original) */
    {
        GFX_RGB(0x05, 0x05, 0x05), GFX_RGB(0x10, 0x12, 0x14),
        GFX_RGB(0x1A, 0x1D, 0x20), GFX_RGB(0x2A, 0x2E, 0x32),
        GFX_RGB(0xF2, 0xF0, 0xEB), GFX_RGB(0x8C, 0x91, 0x96),
        GFX_RGB(0xC8, 0x10, 0x2E), GFX_RGB(0x00, 0x8C, 0x45),
        GFX_RGB(0xFF, 0xB0, 0x00), GFX_RGB(0xFF, 0x2A, 0x2A),
        GFX_RGB(0x2E, 0xCC, 0x71), GFX_RGB(0x00, 0xC2, 0xD1)
    },
    /* 1 - Verde Quadrifoglio */
    {
        GFX_RGB(0x04, 0x06, 0x05), GFX_RGB(0x10, 0x14, 0x12),
        GFX_RGB(0x18, 0x20, 0x1B), GFX_RGB(0x28, 0x34, 0x2C),
        GFX_RGB(0xF0, 0xF5, 0xF0), GFX_RGB(0x88, 0x96, 0x8C),
        GFX_RGB(0x00, 0x9A, 0x50), GFX_RGB(0x7A, 0xC1, 0x43),
        GFX_RGB(0xF0, 0xB4, 0x20), GFX_RGB(0xFF, 0x2A, 0x2A),
        GFX_RGB(0x3E, 0xD8, 0x7E), GFX_RGB(0x30, 0xC8, 0xB0)
    },
    /* 2 - Ambar */
    {
        GFX_RGB(0x06, 0x05, 0x03), GFX_RGB(0x15, 0x12, 0x09),
        GFX_RGB(0x1F, 0x19, 0x0D), GFX_RGB(0x35, 0x2B, 0x18),
        GFX_RGB(0xF5, 0xF0, 0xE4), GFX_RGB(0x99, 0x8B, 0x72),
        GFX_RGB(0xFF, 0x8C, 0x00), GFX_RGB(0xFF, 0xC4, 0x2A),
        GFX_RGB(0xFF, 0xB0, 0x00), GFX_RGB(0xFF, 0x2A, 0x2A),
        GFX_RGB(0x2E, 0xCC, 0x71), GFX_RGB(0xF0, 0xC8, 0x60)
    },
    /* 3 - Azzurro */
    {
        GFX_RGB(0x03, 0x05, 0x07), GFX_RGB(0x0C, 0x13, 0x18),
        GFX_RGB(0x14, 0x1D, 0x24), GFX_RGB(0x22, 0x2F, 0x3A),
        GFX_RGB(0xEE, 0xF4, 0xF8), GFX_RGB(0x86, 0x94, 0x9E),
        GFX_RGB(0x00, 0x8F, 0xC8), GFX_RGB(0x00, 0xC2, 0xD1),
        GFX_RGB(0xF2, 0xB0, 0x30), GFX_RGB(0xFF, 0x36, 0x46),
        GFX_RGB(0x2E, 0xD0, 0x9A), GFX_RGB(0x40, 0xD8, 0xE8)
    },
    /* 4 - Mono */
    {
        GFX_RGB(0x05, 0x05, 0x05), GFX_RGB(0x12, 0x12, 0x12),
        GFX_RGB(0x1C, 0x1C, 0x1C), GFX_RGB(0x30, 0x30, 0x30),
        GFX_RGB(0xF2, 0xF2, 0xF2), GFX_RGB(0x94, 0x94, 0x94),
        GFX_RGB(0xE4, 0xE4, 0xE4), GFX_RGB(0xA8, 0xA8, 0xA8),
        GFX_RGB(0xC8, 0xC8, 0xC8), GFX_RGB(0xFF, 0x3A, 0x3A),
        GFX_RGB(0xD0, 0xD0, 0xD0), GFX_RGB(0xB8, 0xB8, 0xB8)
    },
    /* 5 - Forza Horizon: morado de festival, rosa y cian. */
    {
        GFX_RGB(0x0B, 0x06, 0x14), GFX_RGB(0x1A, 0x0F, 0x2B),
        GFX_RGB(0x2A, 0x17, 0x45), GFX_RGB(0x4A, 0x2A, 0x73),
        GFX_RGB(0xFF, 0xFF, 0xFF), GFX_RGB(0xB9, 0xA8, 0xD0),
        GFX_RGB(0xFF, 0x2E, 0x63), GFX_RGB(0x00, 0xE5, 0xA0),
        GFX_RGB(0xFF, 0xB0, 0x20), GFX_RGB(0xFF, 0x3B, 0x30),
        GFX_RGB(0x00, 0xE5, 0xA0), GFX_RGB(0x00, 0xD9, 0xFF)
    },
    /* 6 - NFS Underground: neon verde, azul electrico y morado. */
    {
        GFX_RGB(0x04, 0x06, 0x08), GFX_RGB(0x08, 0x12, 0x1C),
        GFX_RGB(0x0E, 0x22, 0x33), GFX_RGB(0x1C, 0x44, 0x5E),
        GFX_RGB(0xE8, 0xFB, 0xFF), GFX_RGB(0x6F, 0xA8, 0xB8),
        GFX_RGB(0x39, 0xFF, 0x14), GFX_RGB(0x00, 0xA2, 0xFF),
        GFX_RGB(0xFF, 0xD4, 0x00), GFX_RGB(0xFF, 0x2E, 0x2E),
        GFX_RGB(0x39, 0xFF, 0x14), GFX_RGB(0x9B, 0x5C, 0xFF)
    },
    /* 7 - Gran Turismo: azul profundo, blanco y rojo de competicion. */
    {
        GFX_RGB(0x03, 0x06, 0x0D), GFX_RGB(0x0A, 0x14, 0x24),
        GFX_RGB(0x12, 0x22, 0x39), GFX_RGB(0x28, 0x40, 0x5E),
        GFX_RGB(0xF2, 0xF7, 0xFF), GFX_RGB(0x8F, 0xA6, 0xC0),
        GFX_RGB(0xE4, 0x00, 0x2B), GFX_RGB(0x00, 0xC4, 0x8C),
        GFX_RGB(0xFF, 0xC2, 0x4B), GFX_RGB(0xFF, 0x33, 0x55),
        GFX_RGB(0x00, 0xC4, 0x8C), GFX_RGB(0x4F, 0xC3, 0xF7)
    },
    /* 8 - Initial D: noche de montana, blanco/negro del AE86 y rojo. */
    {
        GFX_RGB(0x04, 0x05, 0x07), GFX_RGB(0x0E, 0x11, 0x15),
        GFX_RGB(0x1A, 0x1E, 0x24), GFX_RGB(0x36, 0x3C, 0x45),
        GFX_RGB(0xF7, 0xF9, 0xFC), GFX_RGB(0x9A, 0xA3, 0xAD),
        GFX_RGB(0xE6, 0x00, 0x12), GFX_RGB(0x3D, 0xDC, 0x84),
        GFX_RGB(0xFF, 0xC4, 0x00), GFX_RGB(0xFF, 0x2D, 0x2D),
        GFX_RGB(0x3D, 0xDC, 0x84), GFX_RGB(0x5C, 0xC8, 0xFF)
    },
    /* 9 - Cyberpunk: neon sobre negro violaceo. */
    {
        GFX_RGB(0x05, 0x00, 0x10), GFX_RGB(0x12, 0x00, 0x24),
        GFX_RGB(0x1E, 0x00, 0x38), GFX_RGB(0x3A, 0x00, 0x66),
        GFX_RGB(0x00, 0xFF, 0xFF), GFX_RGB(0xFF, 0x00, 0xFF),
        GFX_RGB(0xFF, 0x00, 0x66), GFX_RGB(0x00, 0xFF, 0x66),
        GFX_RGB(0xFF, 0xFF, 0x00), GFX_RGB(0xFF, 0x00, 0xFF),
        GFX_RGB(0x00, 0xFF, 0xCC), GFX_RGB(0x00, 0xFF, 0xFF)
    },
    /* 10 - Retro 80s: CRT verde sobre negro. */
    {
        GFX_RGB(0x00, 0x0A, 0x00), GFX_RGB(0x00, 0x18, 0x00),
        GFX_RGB(0x00, 0x24, 0x00), GFX_RGB(0x00, 0x38, 0x00),
        GFX_RGB(0x33, 0xFF, 0x33), GFX_RGB(0x1A, 0x99, 0x1A),
        GFX_RGB(0xFF, 0x33, 0x33), GFX_RGB(0x33, 0xFF, 0x33),
        GFX_RGB(0xFF, 0xAA, 0x00), GFX_RGB(0xFF, 0x00, 0xFF),
        GFX_RGB(0x33, 0xFF, 0x33), GFX_RGB(0x00, 0xFF, 0xFF)
    },
    /* 11 - Nothing: negro y blanco minimalista con acento rojo. */
    {
        GFX_RGB(0x12, 0x12, 0x12), GFX_RGB(0x1E, 0x1E, 0x1E),
        GFX_RGB(0x2A, 0x2A, 0x2A), GFX_RGB(0x3A, 0x3A, 0x3A),
        GFX_RGB(0xF2, 0xF2, 0xF2), GFX_RGB(0x8A, 0x8A, 0x8A),
        GFX_RGB(0xFF, 0x00, 0x00), GFX_RGB(0xF2, 0xF2, 0xF2),
        GFX_RGB(0xFF, 0xD4, 0x00), GFX_RGB(0x9B, 0x5C, 0xFF),
        GFX_RGB(0xF2, 0xF2, 0xF2), GFX_RGB(0xF2, 0xF2, 0xF2)
    },
    /* 12 - Back to the Future: flux capacitor cyan/violeta. */
    {
        GFX_RGB(0x02, 0x00, 0x10), GFX_RGB(0x0A, 0x00, 0x2A),
        GFX_RGB(0x14, 0x00, 0x3E), GFX_RGB(0x2A, 0x00, 0x66),
        GFX_RGB(0x00, 0xFF, 0xFF), GFX_RGB(0x9B, 0x5C, 0xFF),
        GFX_RGB(0xFF, 0x00, 0x66), GFX_RGB(0x00, 0xFF, 0x66),
        GFX_RGB(0xFF, 0xFF, 0x00), GFX_RGB(0x9B, 0x5C, 0xFF),
        GFX_RGB(0x00, 0xFF, 0xCC), GFX_RGB(0x00, 0xFF, 0xFF)
    },
    /* 13 - Todo gas: calle y NOS naranja/azul. */
    {
        GFX_RGB(0x00, 0x05, 0x10), GFX_RGB(0x00, 0x10, 0x24),
        GFX_RGB(0x00, 0x1A, 0x38), GFX_RGB(0x00, 0x2A, 0x5A),
        GFX_RGB(0xFF, 0xFF, 0xFF), GFX_RGB(0x6F, 0xA8, 0xB8),
        GFX_RGB(0xFF, 0x66, 0x00), GFX_RGB(0x00, 0xA2, 0xFF),
        GFX_RGB(0xFF, 0xD4, 0x00), GFX_RGB(0xFF, 0x2E, 0x2E),
        GFX_RGB(0x00, 0xA2, 0xFF), GFX_RGB(0xFF, 0x66, 0x00)
    }
};

static const char *const g_theme_names[GFX_THEME_COUNT] = {
    "QV ROSSO", "VERDE", "AMBAR", "AZZURRO", "MONO",
    "FORZA", "NFS", "GRAN TURISMO", "INITIAL D",
    "CYBERPUNK", "RETRO 80", "NOTHING",
    "BACK FUTURE", "TODO GAS"
};

gfx_theme g_active_theme;

/* Estilo tipografico asociado al tema activo:
 * 5 FORZA = negrita, 6 NFS = cursiva/neon, 7 GT = sombra limpia,
 * 8 INITIAL D = sombra retro. El resto usa la fuente normal. */
static int g_text_style = 0;

static int clamp_byte(int v)
{
    return v < 0 ? 0 : (v > 255 ? 255 : v);
}

/* Escala un color manteniendo el alfa. */
uint32_t gfx_shade(uint32_t c, int percent)
{
    int r = clamp_byte((int)(c & 0xFF) * percent / 100);
    int g = clamp_byte((int)((c >> 8) & 0xFF) * percent / 100);
    int b = clamp_byte((int)((c >> 16) & 0xFF) * percent / 100);
    return GFX_RGBA(r, g, b, (int)((c >> 24) & 0xFF));
}

static uint32_t shade_color(uint32_t c, int percent)
{
    return gfx_shade(c, percent);
}

void gfx_theme_apply(int preset, int contrast)
{
    if (preset < 0 || preset >= GFX_THEME_COUNT) {
        preset = 0;
    }
    if (contrast < 0 || contrast > 2) {
        contrast = 1;
    }

    g_active_theme = g_theme_presets[preset];
    g_text_style = preset;

    if (contrast == 0) {
        /* Bajo: menos contraste, todo un poco mas apagado. */
        g_active_theme.bg = GFX_RGB(0x0A, 0x0A, 0x0A);
        g_active_theme.panel = shade_color(g_active_theme.panel, 85);
        g_active_theme.panel2 = shade_color(g_active_theme.panel2, 85);
        g_active_theme.track = shade_color(g_active_theme.track, 75);
        g_active_theme.text = shade_color(g_active_theme.text, 78);
        g_active_theme.dim = shade_color(g_active_theme.dim, 70);
        g_active_theme.red = shade_color(g_active_theme.red, 85);
        g_active_theme.green = shade_color(g_active_theme.green, 85);
        g_active_theme.amber = shade_color(g_active_theme.amber, 85);
        g_active_theme.crit = shade_color(g_active_theme.crit, 90);
        g_active_theme.ok = shade_color(g_active_theme.ok, 85);
        g_active_theme.cyan = shade_color(g_active_theme.cyan, 85);
    } else if (contrast == 2) {
        /* Alto: fondo negro puro y trazos mas luminosos. */
        g_active_theme.bg = GFX_RGB(0x00, 0x00, 0x00);
        g_active_theme.panel = shade_color(g_active_theme.panel, 125);
        g_active_theme.panel2 = shade_color(g_active_theme.panel2, 130);
        g_active_theme.track = shade_color(g_active_theme.track, 135);
        g_active_theme.text = shade_color(g_active_theme.text, 105);
        g_active_theme.dim = shade_color(g_active_theme.dim, 118);
        g_active_theme.red = shade_color(g_active_theme.red, 115);
        g_active_theme.green = shade_color(g_active_theme.green, 115);
        g_active_theme.amber = shade_color(g_active_theme.amber, 110);
        g_active_theme.crit = shade_color(g_active_theme.crit, 110);
        g_active_theme.ok = shade_color(g_active_theme.ok, 115);
        g_active_theme.cyan = shade_color(g_active_theme.cyan, 115);
    }
}

const char *gfx_theme_name(int preset)
{
    if (preset < 0 || preset >= GFX_THEME_COUNT) {
        preset = 0;
    }
    return g_theme_names[preset];
}

#define BUF_WIDTH 512
#define VERTEX_FORMAT (GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_2D)

/* 32768 vertices = 5461 quads por frame; de sobra para este dashboard. */
#define VBUF_VERTS 32768

typedef struct {
    unsigned int color;
    float x;
    float y;
    float z;
} Vertex;

/* La lista solo lleva comandos (estado + un DrawArray por frame). */
static unsigned int __attribute__((aligned(16))) g_list[16384];
static Vertex __attribute__((aligned(16))) g_vbuf[VBUF_VERTS];
static int g_vcount = 0;

/* Metricas del ultimo frame (para el modo DEBUG de main.c). */
static int g_last_vertex_count = 0;
static uint32_t g_last_wait_us = 0;

static void *g_disp = (void *)0x88000;

/* El caracter 0x7E se sustituye por el simbolo de grado. */
static const unsigned char g_degree_glyph[8] =
    { 0x1C, 0x22, 0x22, 0x1C, 0x00, 0x00, 0x00, 0x00 };

static void push_vertex(float x, float y, uint32_t color)
{
    if (g_vcount >= VBUF_VERTS) {
        return;
    }
    Vertex *v = &g_vbuf[g_vcount++];
    v->color = color;
    v->x = x;
    v->y = y;
    v->z = 0.0f;
}

void gfx_rect(float x, float y, float w, float h, uint32_t color)
{
    if (w <= 0.0f || h <= 0.0f) {
        return;
    }
    if (g_vcount + 6 > VBUF_VERTS) {
        return;
    }

    float x1 = x + w;
    float y1 = y + h;

    push_vertex(x, y, color);
    push_vertex(x1, y, color);
    push_vertex(x1, y1, color);

    push_vertex(x, y, color);
    push_vertex(x1, y1, color);
    push_vertex(x, y1, color);
}

void gfx_border(float x, float y, float w, float h, float t, uint32_t color)
{
    if (t <= 0.0f || w <= 0.0f || h <= 0.0f) {
        return;
    }
    gfx_rect(x, y, w, t, color);
    gfx_rect(x, y + h - t, w, t, color);
    gfx_rect(x, y + t, t, h - 2.0f * t, color);
    gfx_rect(x + w - t, y + t, t, h - 2.0f * t, color);
}

void gfx_line(float x1, float y1, float x2, float y2, float t, uint32_t color)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);

    if (len < 0.001f || t <= 0.0f) {
        return;
    }
    if (g_vcount + 6 > VBUF_VERTS) {
        return;
    }

    float nx = (-dy / len) * (t * 0.5f);
    float ny = (dx / len) * (t * 0.5f);

    float ax = x1 + nx, ay = y1 + ny;
    float bx = x2 + nx, by = y2 + ny;
    float cx = x2 - nx, cy = y2 - ny;
    float ex = x1 - nx, ey = y1 - ny;

    push_vertex(ax, ay, color);
    push_vertex(bx, by, color);
    push_vertex(cx, cy, color);

    push_vertex(ax, ay, color);
    push_vertex(cx, cy, color);
    push_vertex(ex, ey, color);
}

void gfx_circle(float cx, float cy, float r, uint32_t color)
{
    int ir = (int)(r + 0.5f);

    for (int dy = -ir; dy <= ir; dy++) {
        float d = (float)dy;
        float inside = r * r - d * d;
        if (inside <= 0.0f) {
            continue;
        }
        float half = sqrtf(inside);
        gfx_rect(cx - half, cy + d, half * 2.0f, 1.0f, color);
    }
}

void gfx_arc(float cx, float cy, float r, float t,
             float start_deg, float end_deg, int segments, uint32_t color)
{
    if (segments < 1) {
        segments = 1;
    }

    const float deg2rad = 0.017453292519943295f;
    float a0 = start_deg * deg2rad;
    float step = (end_deg - start_deg) * deg2rad / (float)segments;

    float px = cx + r * cosf(a0);
    float py = cy + r * sinf(a0);

    for (int i = 1; i <= segments; i++) {
        float a = a0 + step * (float)i;
        float nx = cx + r * cosf(a);
        float ny = cy + r * sinf(a);
        gfx_line(px, py, nx, ny, t, color);
        px = nx;
        py = ny;
    }
}

/* Dibuja una pasada de texto. italic inclina la fuente fila a fila y
 * bold duplica el ultimo pixel de cada tramo horizontal. */
static int text_pass(int x, int y, const char *s, uint32_t color, int scale,
                     int italic, int bold)
{
    int cx = x;
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        unsigned char c = *p;
        if (c < 32 || c > 127) {
            c = '?';
        }

        const unsigned char *glyph =
            (c == 0x7E) ? g_degree_glyph : font8x8_basic[c];
        for (int row = 0; row < 8; row++) {
            unsigned char bits = glyph[row];
            if (bits == 0) {
                continue;
            }

            int shift = italic ? ((7 - row) * scale) / 3 : 0;
            int col = 0;
            while (col < 8) {
                if ((bits & (1u << col)) == 0) {
                    col++;
                    continue;
                }
                int start = col;
                while (col < 8 && (bits & (1u << col)) != 0) {
                    col++;
                }
                gfx_rect((float)(cx + start * scale + shift),
                         (float)(y + row * scale),
                         (float)((col - start) * scale +
                                 (bold ? scale : 0)),
                         (float)scale, color);
            }
        }
        cx += 8 * scale;
    }
    return cx;
}

static int text_pass_dotted(int x, int y, const char *s, uint32_t color,
                            int scale)
{
    int cx = x;
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        unsigned char c = *p;
        if (c < 32 || c > 127) {
            c = '?';
        }

        const unsigned char *glyph =
            (c == 0x7E) ? g_degree_glyph : font8x8_basic[c];
        for (int row = 0; row < 8; row++) {
            unsigned char bits = glyph[row];
            if (bits == 0) {
                continue;
            }
            for (int col = 0; col < 8; col++) {
                if ((bits & (1u << col)) == 0) {
                    continue;
                }
                int px = cx + col * scale;
                int py = y + row * scale;
                int dot = (scale > 1) ? (scale - 1) : 1;
                gfx_rect((float)px, (float)py, (float)dot, (float)dot, color);
            }
        }
        cx += 8 * scale + (scale > 1 ? 2 : 1);
    }
    return cx;
}

int gfx_text(int x, int y, const char *s, uint32_t color, int scale)
{
    if (s == NULL) {
        return x;
    }
    if (scale < 1) {
        scale = 1;
    }

    switch (g_text_style) {
    case 5: /* Forza Horizon: negrita con sombra suave. */
        if (scale >= 2) {
            text_pass(x + 2, y + 2, s, gfx_shade(color, 40), scale, 0, 1);
        }
        return text_pass(x, y, s, color, scale, 0, 1);

    case 6: /* NFS Underground: cursiva neon con doble halo. */
        if (scale >= 2) {
            text_pass(x + 3, y + 3, s, gfx_shade(color, 45), scale, 1, 1);
            text_pass(x + 2, y + 2, s, gfx_shade(color, 22), scale, 1, 1);
        }
        return text_pass(x, y, s, color, scale, 1, 1);

    case 7: /* Gran Turismo: limpia con sombra de una pieza. */
        if (scale >= 2) {
            text_pass(x + 2, y + 2, s, gfx_shade(color, 35), scale, 0, 0);
        }
        return text_pass(x, y, s, color, scale, 0, 0);

    case 8: /* Initial D: sombra dura y cursiva ligera. */
        /* La sombra solo en texto grande: en el texto pequeno (etiquetas,
         * pestanas) multiplicaba por 2 el coste y saturaba los vertices. */
        if (scale >= 2) {
            text_pass(x + 2, y + 2, s, gfx_shade(color, 20), scale, 1, 0);
        }
        return text_pass(x, y, s, color, scale, 1, 0);

    case 9: /* Cyberpunk: brillo neon. */
        /* Sombra y halo solo en texto grande (igual que el resto de temas). */
        if (scale >= 2) {
            text_pass(x + 2, y + 2, s, gfx_shade(color, 25), scale, 0, 0);
            text_pass(x + 1, y + 1, s, gfx_shade(color, 60), scale, 0, 0);
        }
        return text_pass(x, y, s, color, scale, 0, 0);

    case 10: /* Retro: fosforo verde, sin sombra. */
        if (scale >= 2) {
            text_pass(x + 1, y + 1, s, gfx_shade(color, 35), scale, 0, 0);
        }
        return text_pass(x, y, s, color, scale, 0, 0);

    case 11: /* Nothing: texto punteado (dot matrix). */
        return text_pass_dotted(x, y, s, color, scale);

    default:
        return text_pass(x, y, s, color, scale, 0, 0);
    }
}

int gfx_text_width(const char *s, int scale)
{
    if (s == NULL) {
        return 0;
    }
    if (scale < 1) {
        scale = 1;
    }
    return (int)strlen(s) * 8 * scale;
}

void gfx_text_center(int cx, int y, const char *s, uint32_t color, int scale)
{
    gfx_text(cx - gfx_text_width(s, scale) / 2, y, s, color, scale);
}

void gfx_text_right(int rx, int y, const char *s, uint32_t color, int scale)
{
    gfx_text(rx - gfx_text_width(s, scale), y, s, color, scale);
}

int gfx_init(void)
{
    sceGuInit();

    sceGuStart(GU_DIRECT, g_list);
    sceGuDrawBuffer(GU_PSM_8888, (void *)0, BUF_WIDTH);
    sceGuDispBuffer(GFX_W, GFX_H, g_disp, BUF_WIDTH);
    sceGuOffset(2048 - GFX_W / 2, 2048 - GFX_H / 2);
    sceGuViewport(2048, 2048, GFX_W, GFX_H);
    sceGuScissor(0, 0, GFX_W, GFX_H);
    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_TEXTURE_2D);
    sceGuDisable(GU_CULL_FACE);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    sceGuClearColor(QV_BG);
    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);

    sceDisplaySetMode(PSP_DISPLAY_MODE_LCD, GFX_W, GFX_H);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);

    return 0;
}

void gfx_shutdown(void)
{
    sceGuTerm();
}

void gfx_frame_begin(void)
{
    sceGuStart(GU_DIRECT, g_list);

    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_TEXTURE_2D);
    sceGuDisable(GU_CULL_FACE);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

    sceGuClearColor(QV_BG);
    sceGuClear(GU_COLOR_BUFFER_BIT);

    g_vcount = 0;
}

void gfx_frame_end(void)
{
    g_last_vertex_count = g_vcount;

    if (g_vcount > 0) {
        /* La GE no ve la cache de datos: hay que bajar los vertices a RAM. */
        sceKernelDcacheWritebackRange(g_vbuf,
                                      (unsigned int)(g_vcount * (int)sizeof(Vertex)));
        sceGuDrawArray(GU_TRIANGLES, VERTEX_FORMAT, g_vcount, 0, g_vbuf);
    }
    g_vcount = 0;

    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
    sceGuSwapBuffers();

    /* Tiempo real de espera del VBlank: es lo que main.c descuenta para
     * medir solo el tiempo de CPU empleado en preparar el frame. */
    uint32_t t0 = (uint32_t)sceKernelGetSystemTimeWide();
    sceDisplayWaitVblankStart();
    uint32_t t1 = (uint32_t)sceKernelGetSystemTimeWide();
    g_last_wait_us = t1 - t0;
}

int gfx_vertex_count(void)
{
    return g_last_vertex_count;
}

uint32_t gfx_last_wait_us(void)
{
    return g_last_wait_us;
}

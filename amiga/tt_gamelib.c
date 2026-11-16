/* tt_gamelib.c: TunnelTanks' gamelib (src/include/gamelib.h), the layer its
 * author made so the game could leave SDL, on amiga/tt_plat.h:
 *
 *  - the screen is one 320 x 200 frame of 8-bit pixels: the game's
 *    160 x 100 world drawn two by two, exactly filling it; each colour the
 *    game asks for gets a palette entry the first time (it uses about 40);
 *  - a frame is shown each step, 24 a second, as upstream's SDL loop did;
 *  - the keys are upstream's: two players, W A S D + Ctrl and the cursor
 *    keys + /; one player, the cursor keys + Ctrl. The first pad (when one
 *    is connected) also drives tank 1, as upstream's joystick did;
 *  - Esc (or Start + Back on the pad) quits;
 *  - the level pictures of --only-gen and --debug are written as 24-bit
 *    BMP files by hand.
 *
 * GPL-3.0-or-later, like TunnelTanks. Copyright (c) 2026 AmiCradle Team.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <gamelib.h>
#include <types.h>
#include <tank.h>
#include <tweak.h>
#include <memalloc.h>
#include <screen.h>

#include "tt_plat.h"
#include "tt_gamelib.h"

/* Amiga raw key codes */
#define RAW_W      0x11
#define RAW_A      0x20
#define RAW_S      0x21
#define RAW_D      0x22
#define RAW_SLASH  0x3A
#define RAW_KP0    0x0F
#define RAW_ESC    0x45
#define RAW_UP     0x4C
#define RAW_DOWN   0x4D
#define RAW_RIGHT  0x4E
#define RAW_LEFT   0x4F
#define RAW_CTRL   0x63

static unsigned char frame[TT_W * TT_H];
static unsigned char pal[256 * 3];
static int colours, is_open, quit_seen;
static struct tt_input input;

/* ---- colours ---- */

int tt_colour_index(Color c)
{
    static int last = -1;
    int i;
    if (last >= 0 && pal[last * 3] == c.r && pal[last * 3 + 1] == c.g && pal[last * 3 + 2] == c.b)
        return last;
    for (i = 0; i < colours; i++)
        if (pal[i * 3] == c.r && pal[i * 3 + 1] == c.g && pal[i * 3 + 2] == c.b)
            return last = i;
    if (colours < 256) {
        pal[colours * 3] = c.r;
        pal[colours * 3 + 1] = c.g;
        pal[colours * 3 + 2] = c.b;
        return last = colours++;
    }
    /* full (never, with this game's colours): the nearest */
    {
        int best = 0;
        long bd = 1L << 30;
        for (i = 0; i < 256; i++) {
            long dr = pal[i * 3] - c.r, dg = pal[i * 3 + 1] - c.g, db = pal[i * 3 + 2] - c.b;
            long d = dr * dr + dg * dg + db * db;
            if (d < bd) { bd = d; best = i; }
        }
        return last = best;
    }
}

int tt_colour_count(void) { return colours; }
const unsigned char *tt_frame(void) { return frame; }
const unsigned char *tt_palette(void) { return pal; }

/* ---- the gamelib ---- */

int gamelib_init(void)
{
    memset(frame, 0, sizeof(frame));
    colours = 0;
    quit_seen = 0;
    memset(&input, 0, sizeof(input));
    return 0;
}

int gamelib_exit(void)
{
    if (is_open) tt_plat_close();
    is_open = 0;
    return 0;
}

int gamelib_get_max_players(void)    { return 2; }
int gamelib_get_can_resize(void)     { return 0; }
int gamelib_get_can_fullscreen(void) { return 0; }
int gamelib_get_can_window(void)     { return 1; }
int gamelib_get_target_fps(void)     { return GAME_FPS; }

void gamelib_main_loop(draw_func func, void *data)
{
    unsigned long next = tt_plat_ms();
    for (;;) {
        unsigned long now;
        tt_plat_input(&input);
        if (input.raw[RAW_ESC] || (input.pad & TT_PAD_QUIT)) quit_seen = 1;
        if (func(data)) return;
        tt_plat_show(frame, pal);
        /* upstream's smart_wait: on to the next 1/24 s */
        next += GAME_FPS_WAIT;
        now = tt_plat_ms();
        if (now < next) tt_plat_idle(next - now);
        else if (now - next > 4 * GAME_FPS_WAIT) next = now;   /* far behind: drop it */
    }
}

/* the keyboard (and the pad) as one of upstream's controllers */
typedef struct TTKeys {
    unsigned char left, right, up, down, shoot, shoot2;
    int pad;
} TTKeys;

static void keys_controller(PublicTankInfo *i, void *d, int *vx, int *vy, unsigned *s)
{
    TTKeys *k = d;
    unsigned p = k->pad && input.pad_connected ? input.pad : 0;
    (void)i;
    *vx = (input.raw[k->right] || (p & TT_PAD_RIGHT)) - (input.raw[k->left] || (p & TT_PAD_LEFT));
    *vy = (input.raw[k->down] || (p & TT_PAD_DOWN)) - (input.raw[k->up] || (p & TT_PAD_UP));
    *s = input.raw[k->shoot] || (k->shoot2 && input.raw[k->shoot2]) || (p & TT_PAD_FIRE);
}

static void attach(Tank *t, unsigned char l, unsigned char r, unsigned char u, unsigned char dn,
                   unsigned char s1, unsigned char s2, int pad)
{
    TTKeys *k = get_object(TTKeys);
    k->left = l; k->right = r; k->up = u; k->down = dn;
    k->shoot = s1; k->shoot2 = s2; k->pad = pad;
    tank_set_controller(t, keys_controller, k);
}

int gamelib_tank_attach(Tank *t, int tank_num, int num_players)
{
    if (num_players == 1 && tank_num == 0)
        attach(t, RAW_LEFT, RAW_RIGHT, RAW_UP, RAW_DOWN, RAW_CTRL, 0, 1);
    else if (num_players == 2 && tank_num == 0)
        attach(t, RAW_A, RAW_D, RAW_W, RAW_S, RAW_CTRL, 0, 1);
    else if (num_players == 2 && tank_num == 1)
        attach(t, RAW_LEFT, RAW_RIGHT, RAW_UP, RAW_DOWN, RAW_SLASH, RAW_KP0, 0);
    else
        return 1;
    return 0;
}

EventType gamelib_event_get_type(void)
{
    return quit_seen ? GAME_EVENT_EXIT : GAME_EVENT_NONE;
}

Rect gamelib_event_resize_get_size(void) { return RECT(0, 0, 0, 0); }
void gamelib_event_done(void) { quit_seen = 0; }

int gamelib_set_fullscreen(void) { return 1; }

int gamelib_set_window(unsigned w, unsigned h)
{
    (void)w; (void)h;
    if (is_open) return 0;
    if (tt_plat_open() != 0) {
        gamelib_error("TunnelTanks: could not open a 320 x 200 256-colour screen\n");
        exit(1);
    }
    is_open = 1;
    return 0;
}

Rect gamelib_get_resolution(void)
{
    return RECT(0, 0, TT_W, TT_H);
}

int gamelib_get_fullscreen(void) { return 0; }

int gamelib_draw_box(Rect *rect, Color c)
{
    unsigned char v = (unsigned char)tt_colour_index(c);
    int x0, y0, x1, y1, y;
    if (!rect) {
        memset(frame, v, sizeof(frame));
        return 0;
    }
    x0 = rect->x; y0 = rect->y;
    x1 = x0 + (int)rect->w; y1 = y0 + (int)rect->h;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > TT_W) x1 = TT_W;
    if (y1 > TT_H) y1 = TT_H;
    if (x0 >= x1 || y0 >= y1) return 0;
    if (x1 - x0 == 2) {   /* the usual: one of the game's pixels */
        for (y = y0; y < y1; y++) {
            unsigned char *p = frame + y * TT_W + x0;
            p[0] = v; p[1] = v;
        }
        return 0;
    }
    for (y = y0; y < y1; y++) memset(frame + y * TT_W + x0, v, (size_t)(x1 - x0));
    return 0;
}

/* ---- level pictures (--only-gen, --debug): plain 24-bit BMP ---- */

struct BMPFile {
    unsigned w, h;
    unsigned char *px;   /* rows bottom up, b g r, padded to 4 */
};

static unsigned bmp_row(unsigned w) { return (w * 3 + 3) & ~3u; }

BMPFile *gamelib_bmp_new(unsigned width, unsigned height)
{
    BMPFile *f = get_object(BMPFile);
    f->w = width; f->h = height;
    f->px = calloc(1, (size_t)bmp_row(width) * height);
    return f;
}

void gamelib_bmp_set_pixel(BMPFile *f, unsigned x, unsigned y, Color c)
{
    unsigned char *p;
    if (!f->px || x >= f->w || y >= f->h) return;
    p = f->px + (size_t)(f->h - 1 - y) * bmp_row(f->w) + x * 3;
    p[0] = c.b; p[1] = c.g; p[2] = c.r;
}

static void le(unsigned char *p, unsigned long v, int n)
{
    while (n--) { *p++ = (unsigned char)v; v >>= 8; }
}

void gamelib_bmp_finalize(BMPFile *f, char *filename)
{
    unsigned char h[54];
    unsigned long size = (unsigned long)bmp_row(f->w) * f->h;
    FILE *o;
    memset(h, 0, sizeof(h));
    h[0] = 'B'; h[1] = 'M';
    le(h + 2, 54 + size, 4);
    le(h + 10, 54, 4);
    le(h + 14, 40, 4);
    le(h + 18, f->w, 4);
    le(h + 22, f->h, 4);
    le(h + 26, 1, 2);
    le(h + 28, 24, 2);
    le(h + 34, size, 4);
    o = fopen(filename, "wb");
    if (o && f->px) {
        fwrite(h, 1, sizeof(h), o);
        fwrite(f->px, 1, size, o);
    }
    if (o) fclose(o);
    free(f->px);
    free_mem(f);
}

/* ---- messages ---- */

void gamelib_print(char *str, ...)
{
    va_list l;
    va_start(l, str);
    vfprintf(stdout, str, l);
    va_end(l);
}

void gamelib_debug(char *str, ...)
{
    va_list l;
    va_start(l, str);
    vfprintf(stderr, str, l);
    va_end(l);
}

void gamelib_error(char *str, ...)
{
    va_list l;
    va_start(l, str);
    vfprintf(stderr, str, l);
    va_end(l);
}

/* no on-screen controls (that was Android's) */
Rect gamelib_gui_get_size(void) { return RECT(0, 0, 0, 0); }
void gamelib_gui_draw(Screen *s, Rect r) { (void)s; (void)r; }

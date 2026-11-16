/* unit_test.c: amiga/tt_gamelib.c on a PC, one check at a time: colours
 * to palette entries, boxes into the 320 x 200 frame (clipped), the keys
 * and the pad as each tank's controller for one and two players, Esc and
 * Start + Back as the game's exit event, the level pictures as BMP files.
 * Linked with upstream's memalloc.c, its own tt_plat_* and a
 * tank_set_controller that keeps what it is given.
 * GPL-3.0-or-later, like TunnelTanks. Copyright (c) 2026 AmiCradle Team.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gamelib.h>
#include <tank.h>
#include <tweak.h>

#include "tt_plat.h"
#include "tt_gamelib.h"

static int fails, checks;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

/* the machine */
static struct tt_input next_in;
static int opened, shows;
static unsigned long now;
int tt_plat_open(void) { opened++; return 0; }
void tt_plat_close(void) { opened--; }
void tt_plat_show(const unsigned char *p, const unsigned char *pal) { (void)p; (void)pal; shows++; }
void tt_plat_input(struct tt_input *in) { *in = next_in; }
unsigned long tt_plat_ms(void) { return now; }
void tt_plat_idle(unsigned long ms) { now += ms; }

/* upstream's tank, as far as the controllers go */
static TankController ctl[4];
static void *ctl_data[4];
static int ctl_n;
void tank_set_controller(Tank *t, TankController f, void *d)
{
    (void)t;
    if (ctl_n < 4) { ctl[ctl_n] = f; ctl_data[ctl_n] = d; ctl_n++; }
}

/* one step of the game loop with these keys and pad, then each controller */
static int vx[4], vy[4];
static unsigned sh[4];
static int one_step(void *d) { (void)d; return 1; }
static int two_left;
static int two_steps(void *d) { (void)d; return --two_left == 0; }
static void step(const unsigned char *down, int ndown, unsigned pad, int pad_connected)
{
    int i;
    memset(&next_in, 0, sizeof(next_in));
    for (i = 0; i < ndown; i++) next_in.raw[down[i]] = 1;
    next_in.pad = pad;
    next_in.pad_connected = pad_connected;
    gamelib_main_loop(one_step, 0);
    for (i = 0; i < ctl_n; i++) {
        vx[i] = vy[i] = 0; sh[i] = 0;
        ctl[i](0, ctl_data[i], &vx[i], &vy[i], &sh[i]);
    }
}

static int px(int x, int y) { return tt_frame()[y * TT_W + x]; }

int main(void)
{
    Rect r;
    int a, b;

    gamelib_init();

    /* colours */
    a = tt_colour_index(COLOR(0xc3, 0x79, 0x30));
    b = tt_colour_index(COLOR(0x9a, 0x9a, 0x9a));
    CHECK(a == 0 && b == 1);
    CHECK(tt_colour_index(COLOR(0xc3, 0x79, 0x30)) == a);
    CHECK(tt_colour_count() == 2);
    CHECK(tt_palette()[3] == 0x9a && tt_palette()[4] == 0x9a && tt_palette()[5] == 0x9a);
    {
        int i;
        for (i = 0; i < 300; i++) tt_colour_index(COLOR(i & 255, (i >> 8) * 7, 3));
        CHECK(tt_colour_count() == 256);
        CHECK(tt_colour_index(COLOR(0x9a, 0x9a, 0x9a)) == b);       /* still found */
        CHECK(tt_colour_index(COLOR(0xc2, 0x79, 0x31)) == a);       /* the nearest */
    }
    gamelib_init();
    CHECK(tt_colour_count() == 0);

    /* the screen and boxes */
    CHECK(gamelib_set_window(640, 400) == 0 && opened == 1);
    CHECK(gamelib_set_window(640, 400) == 0 && opened == 1);   /* once */
    r = gamelib_get_resolution();
    CHECK(r.w == TT_W && r.h == TT_H && r.w == 2 * GAME_WIDTH && r.h == 2 * GAME_HEIGHT);
    CHECK(!gamelib_get_fullscreen() && !gamelib_get_can_resize() && gamelib_get_can_window());
    CHECK(gamelib_get_max_players() == 2 && gamelib_get_target_fps() == GAME_FPS);
    gamelib_draw_box(0, COLOR(0, 0, 0));
    a = tt_colour_index(COLOR(0, 0, 0));
    CHECK(px(0, 0) == a && px(319, 199) == a);
    r = RECT(10, 20, 2, 2);
    gamelib_draw_box(&r, COLOR(0, 0, 0xb6));
    b = tt_colour_index(COLOR(0, 0, 0xb6));
    CHECK(b != a);
    CHECK(px(10, 20) == b && px(11, 21) == b && px(12, 20) == a && px(10, 22) == a && px(9, 20) == a);
    r = RECT(-5, -5, 10, 10);
    gamelib_draw_box(&r, COLOR(0, 0, 0xb6));
    CHECK(px(0, 0) == b && px(4, 4) == b && px(5, 5) == a);
    r = RECT(315, 195, 20, 20);
    gamelib_draw_box(&r, COLOR(0, 0, 0xb6));
    CHECK(px(319, 199) == b && px(314, 199) == a);
    r = RECT(400, 10, 4, 4);
    CHECK(gamelib_draw_box(&r, COLOR(1, 2, 3)) == 0);           /* off the screen */

    /* two players: tank 1 W A S D + Ctrl (and the pad), tank 2 cursors + / or keypad 0 */
    ctl_n = 0;
    CHECK(gamelib_tank_attach(0, 0, 2) == 0 && gamelib_tank_attach(0, 1, 2) == 0);
    CHECK(gamelib_tank_attach(0, 2, 2) != 0);
    CHECK(ctl_n == 2);
    {
        unsigned char k1[] = { 0x11, 0x22, 0x63 };          /* W D Ctrl */
        unsigned char k2[] = { 0x4F, 0x4D, 0x3A };          /* left down / */
        unsigned char k3[] = { 0x20, 0x21, 0x0F };          /* A S keypad 0 */
        step(k1, 3, 0, 0);
        CHECK(vx[0] == 1 && vy[0] == -1 && sh[0] && vx[1] == 0 && vy[1] == 0 && !sh[1]);
        step(k2, 3, 0, 0);
        CHECK(vx[1] == -1 && vy[1] == 1 && sh[1] && vx[0] == 0 && vy[0] == 0 && !sh[0]);
        step(k3, 3, 0, 0);
        CHECK(vx[0] == -1 && vy[0] == 1 && !sh[0] && sh[1]);
        step(0, 0, TT_PAD_LEFT | TT_PAD_UP | TT_PAD_FIRE, 1);
        CHECK(vx[0] == -1 && vy[0] == -1 && sh[0] && vx[1] == 0 && !sh[1]);
        step(0, 0, TT_PAD_RIGHT, 0);                          /* no pad: nothing */
        CHECK(vx[0] == 0 && !sh[0]);
        step(k1, 1, TT_PAD_DOWN, 1);                          /* W and pad down cancel */
        CHECK(vy[0] == 0);
    }
    /* one player: the cursor keys + Ctrl, and the pad */
    ctl_n = 0;
    CHECK(gamelib_tank_attach(0, 0, 1) == 0 && gamelib_tank_attach(0, 1, 1) != 0 && ctl_n == 1);
    {
        unsigned char k[] = { 0x4E, 0x4C, 0x63 };            /* right up Ctrl */
        step(k, 3, 0, 0);
        CHECK(vx[0] == 1 && vy[0] == -1 && sh[0]);
        step(0, 0, TT_PAD_DOWN, 1);
        CHECK(vy[0] == 1);
    }

    /* exit: Esc, or Start + Back */
    {
        unsigned char esc[] = { 0x45 };
        step(0, 0, 0, 0);
        CHECK(gamelib_event_get_type() == GAME_EVENT_NONE);
        step(esc, 1, 0, 0);
        CHECK(gamelib_event_get_type() == GAME_EVENT_EXIT);
        gamelib_event_done();
        CHECK(gamelib_event_get_type() == GAME_EVENT_NONE);
        step(0, 0, TT_PAD_QUIT, 1);
        CHECK(gamelib_event_get_type() == GAME_EVENT_EXIT);
        gamelib_event_done();
    }

    /* the loop keeps 24 frames a second */
    now = 0; shows = 0; two_left = 2;
    gamelib_main_loop(two_steps, 0);
    CHECK(shows == 1 && now == GAME_FPS_WAIT);

    /* a level picture */
    {
        BMPFile *f = gamelib_bmp_new(3, 2);
        unsigned char buf[100];
        FILE *in;
        size_t n;
        gamelib_bmp_set_pixel(f, 0, 0, COLOR(1, 2, 3));
        gamelib_bmp_set_pixel(f, 2, 1, COLOR(4, 5, 6));
        gamelib_bmp_set_pixel(f, 9, 9, COLOR(7, 7, 7));       /* outside: ignored */
        gamelib_bmp_finalize(f, "unit-test.bmp");
        in = fopen("unit-test.bmp", "rb");
        n = in ? fread(buf, 1, sizeof(buf), in) : 0;
        if (in) fclose(in);
        remove("unit-test.bmp");
        CHECK(n == 54 + 2 * 12);
        CHECK(buf[0] == 'B' && buf[1] == 'M' && buf[2] == 78 && buf[10] == 54 && buf[18] == 3 && buf[22] == 2 && buf[28] == 24);
        /* bottom row first: (2,1) is in the first row, (0,0) starts the second */
        CHECK(buf[54 + 6] == 6 && buf[54 + 7] == 5 && buf[54 + 8] == 4);
        CHECK(buf[54 + 12] == 3 && buf[54 + 13] == 2 && buf[54 + 14] == 1);
    }

    gamelib_exit();
    CHECK(opened == 0);
    printf("unit: %d of %d checks passed\n", checks - fails, checks);
    return fails != 0;
}

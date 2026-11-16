/* tt_plat.h: what amiga/tt_gamelib.c needs from the machine: a 320 x 200
 * 256-colour screen, the keyboard as Amiga raw keys, the first pad, a
 * millisecond clock and a way to wait. amiga/tt_acgame.c gives it on GFOS
 * through ACGame; tests/host/tt_host.c gives it on a PC for the tests.
 *
 * GPL-3.0-or-later, like TunnelTanks. Copyright (c) 2026 AmiCradle Team.
 */
#ifndef TT_PLAT_H
#define TT_PLAT_H

#define TT_W 320
#define TT_H 200

/* the pad, already turned into directions and buttons */
#define TT_PAD_UP    1
#define TT_PAD_DOWN  2
#define TT_PAD_LEFT  4
#define TT_PAD_RIGHT 8
#define TT_PAD_FIRE  16
#define TT_PAD_QUIT  32   /* Start and Back together */

struct tt_input {
    unsigned char raw[128];   /* Amiga raw key codes held down */
    unsigned pad;             /* TT_PAD_* */
    int pad_connected;
};

int tt_plat_open(void);                     /* 0 when the screen is open */
void tt_plat_close(void);
/* the frame (TT_W x TT_H bytes) and its palette (256 x r, g, b) */
void tt_plat_show(const unsigned char *pixels, const unsigned char *pal);
void tt_plat_input(struct tt_input *in);
unsigned long tt_plat_ms(void);
void tt_plat_idle(unsigned long ms);        /* wait about this long */

#endif

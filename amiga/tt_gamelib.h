/* tt_gamelib.h: what amiga/tt_gamelib.c shows the tests.
 * GPL-3.0-or-later, like TunnelTanks. Copyright (c) 2026 AmiCradle Team. */
#ifndef TT_GAMELIB_H
#define TT_GAMELIB_H
#include <types.h>
int tt_colour_index(Color c);
int tt_colour_count(void);
const unsigned char *tt_frame(void);
const unsigned char *tt_palette(void);
#endif

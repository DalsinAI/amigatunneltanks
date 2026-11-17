/* tt_acgame.c: amiga/tt_plat.h on GFOS, through ACGame (acgame.library):
 * the 320 x 200 frame is ACGame's 8-bit screen (an RTG mode, drawn by
 * GFOpenGfx where it is installed, or AGA; no SDL and no GFOpenGPU), the
 * keyboard ACGame's raw key table, and the first pad (GFOpenInput, or
 * lowlevel.library's port 2) its directions and buttons: stick or d-pad
 * moves, A (or B) fires, Start and Back together quit.
 *
 * GPL-3.0-or-later, like TunnelTanks. Copyright (c) 2026 AmiCradle Team.
 */
#include <string.h>

#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>

#include <acgame/platform.h>

#include "tt_plat.h"

/* the level (1000 x 500 cells) is made with deep recursion */
unsigned long __stack = 256 * 1024;

static int screen;

int tt_plat_open(void)
{
    acgame_video_config cfg;
    cfg.width = TT_W;
    cfg.height = TT_H;
    cfg.refresh_hz = 60;
    cfg.mode = ACGAME_VIDEO_INDEXED8;
    cfg.display = ACGAME_DISPLAY_AUTO;
    if (acgame_platform_init(&cfg) != 0) return -1;
    acgame_platform_show_pointer(0);
    screen = 1;
    return 0;
}

void tt_plat_close(void)
{
    if (screen) acgame_platform_shutdown();
    screen = 0;
}

void tt_plat_show(const unsigned char *pixels, const unsigned char *pal)
{
    acgame_indexed_surface *s;
    int y, i;
    if (!screen) return;
    s = acgame_platform_begin_indexed_frame();
    if (!s) return;
    for (y = 0; y < TT_H && y < (int)s->height; y++)
        memcpy(s->pixels + (size_t)y * s->pitch, pixels + y * TT_W, TT_W);
    for (i = 0; i < 256; i++) {
        s->palette[i].r = pal[i * 3];
        s->palette[i].g = pal[i * 3 + 1];
        s->palette[i].b = pal[i * 3 + 2];
    }
    acgame_platform_present_indexed(s);
}

#define PAD_DEAD 10000   /* upstream's joystick cut-off */

void tt_plat_input(struct tt_input *in)
{
    acgame_input_state st;
    const uint8_t *k;
    uint32_t b;
    memset(in, 0, sizeof(*in));
    if (!screen) return;
    acgame_platform_poll(&st);
    k = acgame_platform_rawkeys();
    if (k) memcpy(in->raw, k, sizeof(in->raw));
    if (!st.pad.connected) return;
    in->pad_connected = 1;
    b = st.pad.buttons;
    if ((b & ACGAME_PAD_BIT(ACGAME_PAD_DPAD_UP)) || st.pad.axes[ACGAME_PAD_LEFTY] < -PAD_DEAD) in->pad |= TT_PAD_UP;
    if ((b & ACGAME_PAD_BIT(ACGAME_PAD_DPAD_DOWN)) || st.pad.axes[ACGAME_PAD_LEFTY] > PAD_DEAD) in->pad |= TT_PAD_DOWN;
    if ((b & ACGAME_PAD_BIT(ACGAME_PAD_DPAD_LEFT)) || st.pad.axes[ACGAME_PAD_LEFTX] < -PAD_DEAD) in->pad |= TT_PAD_LEFT;
    if ((b & ACGAME_PAD_BIT(ACGAME_PAD_DPAD_RIGHT)) || st.pad.axes[ACGAME_PAD_LEFTX] > PAD_DEAD) in->pad |= TT_PAD_RIGHT;
    if (b & (ACGAME_PAD_BIT(ACGAME_PAD_A) | ACGAME_PAD_BIT(ACGAME_PAD_B))) in->pad |= TT_PAD_FIRE;
    if ((b & ACGAME_PAD_BIT(ACGAME_PAD_START)) && (b & ACGAME_PAD_BIT(ACGAME_PAD_BACK))) in->pad |= TT_PAD_QUIT;
}

unsigned long tt_plat_ms(void)
{
    return acgame_platform_ticks_ms();
}

void tt_plat_idle(unsigned long ms)
{
    /* a whole 50 Hz tick or more to wait: give it back to the system */
    if (ms >= 20) Delay(ms / 20);
}

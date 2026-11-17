/* tt_host.c: amiga/tt_plat.h on a PC, for the host tests: the screen is a
 * frame in memory, time is virtual (a wait moves the clock on at once), and
 * the keys and the pad come from scripts.
 *
 *   TT_KEYS   "MS:RAW:1,MS:RAW:0,...": Amiga raw key RAW goes down (1) or
 *             up (0) at virtual time MS
 *   TT_PAD    "MS:BITS,...": from MS the pad's TT_PAD_* bits are BITS (a
 *             pad is connected when TT_PAD is set)
 *   TT_SHOTS  "MS,MS,...": the frame shown at that time is written to
 *             TT_OUT/shot-MS.ppm
 *   TT_END    the virtual time the run ends (exit 0)
 * The run (at TT_END, or when the game quits) writes TT_OUT/summary.txt.
 *
 * GPL-3.0-or-later, like TunnelTanks. Copyright (c) 2026 AmiCradle Team.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tt_plat.h"

static unsigned long vt;
static unsigned char frame[TT_W * TT_H], pal[768];
static int opened, shows, quit;
static const char *out_dir = ".";
static unsigned char keys[128];
static unsigned pad;
static const char *next_key, *next_pad, *next_shot;
static int started;

static unsigned long env_ul(const char *name, unsigned long dflt)
{
    const char *v = getenv(name);
    return v && *v ? strtoul(v, 0, 10) : dflt;
}

static void start(void)
{
    if (started) return;
    started = 1;
    out_dir = getenv("TT_OUT") ? getenv("TT_OUT") : ".";
    next_key = getenv("TT_KEYS");
    next_pad = getenv("TT_PAD");
    next_shot = getenv("TT_SHOTS");
}

static void write_ppm(const char *path)
{
    FILE *f = fopen(path, "wb");
    int i;
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", TT_W, TT_H);
    for (i = 0; i < TT_W * TT_H; i++) fwrite(pal + frame[i] * 3, 1, 3, f);
    fclose(f);
}

static int colours(void)
{
    static unsigned char seen[256];
    int i, n = 0;
    memset(seen, 0, sizeof(seen));
    for (i = 0; i < TT_W * TT_H; i++)
        if (!seen[frame[i]]) { seen[frame[i]] = 1; n++; }
    return n;
}

static void finish(void)
{
    char path[512];
    FILE *f;
    snprintf(path, sizeof(path), "%s/last.ppm", out_dir);
    write_ppm(path);
    snprintf(path, sizeof(path), "%s/summary.txt", out_dir);
    f = fopen(path, "w");
    if (f) {
        fprintf(f, "opened %d\nshows %d\ncolours %d\nvt %lu\nquit %d\n",
                opened, shows, colours(), vt, quit);
        fclose(f);
    }
    fflush(stdout);
    _exit(0);
}

static void run_scripts(void)
{
    unsigned long ms, a, b;
    char *e;
    while (next_key && *next_key) {
        ms = strtoul(next_key, &e, 10);
        if (ms > vt || *e != ':') break;
        a = strtoul(e + 1, &e, 10);
        b = *e == ':' ? strtoul(e + 1, &e, 10) : 0;
        if (a < 128) keys[a] = (unsigned char)b;
        next_key = *e == ',' ? e + 1 : 0;
    }
    while (next_pad && *next_pad) {
        ms = strtoul(next_pad, &e, 10);
        if (ms > vt || *e != ':') break;
        pad = (unsigned)strtoul(e + 1, &e, 10);
        next_pad = *e == ',' ? e + 1 : 0;
    }
}

int tt_plat_open(void) { start(); opened++; return 0; }
void tt_plat_close(void) { quit = 1; finish(); }

void tt_plat_show(const unsigned char *pixels, const unsigned char *p)
{
    memcpy(frame, pixels, sizeof(frame));
    memcpy(pal, p, sizeof(pal));
    shows++;
    while (next_shot && *next_shot) {
        char *e, path[512];
        unsigned long ms = strtoul(next_shot, &e, 10);
        if (ms > vt) break;
        snprintf(path, sizeof(path), "%s/shot-%lu.ppm", out_dir, ms);
        write_ppm(path);
        next_shot = *e == ',' ? e + 1 : 0;
    }
}

void tt_plat_input(struct tt_input *in)
{
    start();
    run_scripts();
    memcpy(in->raw, keys, sizeof(keys));
    in->pad = pad;
    in->pad_connected = getenv("TT_PAD") != 0;
}

unsigned long tt_plat_ms(void) { return vt; }

void tt_plat_idle(unsigned long ms)
{
    vt += ms;
    if (vt >= env_ul("TT_END", 10000)) finish();
}

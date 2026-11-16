# AmigaTunnelTanks

TunnelTanks (Raymond Myers' remake of the DOS game Tunneler; GPL-3.0) for
GFOS (AmigaOS 3.2 with GFOpenUp) on a 68040 or 68060: two players dig and
fight in split windows against computer tanks, on ACGame's 320 x 200
256-colour screen (acgame.library, opened at run time through its stubs
and shipped in the drawer), with no SDL and no GFOpenGPU. Installed as a
GFOpenUp part: `SYS:Games/TunnelTanks`.

Status: builds and plays on a PC and on an emulated 68040 (QEMU) from the
same sources, frame for frame the same (see Tests); the Amiga build is
made by the stove, not yet run in a lab. The game has no sound.

AMIGA_PORT.md is the drawer's ReadMe: the keys, the Shell options, what
it needs, and how the port works.

## Building

    STOVE=~/AmiCradle/stoves/os32-gcc16-open6 OUT=out sh package/build-amiga.sh

or in Cradle, stove_build with the os32-gcc16-open6 stove, target
`package`. Upstream (raydog/tunneltanks) and DalsinAI/acgame are fetched
at the commits `package/tt.lock` pins; nothing of upstream is kept here.
`OUT` gets the archive (`TunnelTanks-0.5-gfos1-68040.lha`, the drawer and
Install-TunnelTanks) and `Parts/TunnelTanks`, installed with

    GFOpenUpTool INSTALL Parts/TunnelTanks

## Tests

    sh package/prepare-source.sh /tmp/tt && TT=/tmp/tt sh tests/host/test_tt.sh

on a PC:

- `tests/host/unit_test.c`: amiga/tt_gamelib.c alone: colours to palette
  entries (and the nearest when all 256 are used), boxes into the frame
  and clipped at its edges, the keys and the pad as each tank's controls
  for one and two players, Esc and Start + Back as the exit, the loop's
  24 frames a second, the level pictures as BMP files (41 checks);
- the whole game on `tests/host/tt_host.c` (a frame in memory, virtual
  time, scripted keys and pad, a fixed seed): both windows and the bars
  drawn 24 times a second; tank 1 and tank 2 each drive their own window
  with their own keys; the pad drives tank 1; one player shows one
  window; Esc quits; each of the four level makers works; the same seed
  gives the same frames.

Big-endian, as on the Amiga, with a 68k Linux cross compiler and QEMU:

    HOSTCC="m68k-linux-gnu-gcc -static" TT_RUN="qemu-m68k -cpu m68040" TT=/tmp/tt \
        TT_SAME_AS=/tmp/tt-pc-frames sh tests/host/test_tt.sh

runs the same tests on an emulated 68040 and checks every frame is byte for
byte the PC's (TT_SAME_AS is a PC run's TT_TEST_KEEP).

## Layout

- `amiga/tt_gamelib.c`: the game's gamelib (upstream's layer for what SDL
  did): the frame, its palette, the loop, the controls, the BMP pictures.
- `amiga/tt_acgame.c`: the screen, keys and pad on ACGame.
  `tests/host/tt_host.c`: on a PC.
- `amiga/patches/0001`: main.c no longer asks for SDL.
- `package/`: the lock, the source preparation, the stove build, the part.

Designed, tested and delivered by The AmiCradle Team.

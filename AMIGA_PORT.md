TunnelTanks 0.5 for GFOS
========================

TunnelTanks (Raymond Myers' remake of Geoffrey Silverton's 1991 DOS
game Tunneler, GPL version 3): tanks dig tunnels through a big
underground level and fight. Each player sees the area round their own
tank in their own window; the other tanks in the level are driven by the
computer. Driving and digging use energy (the yellow bar), being hit
costs health (the blue bar); your base fills both up again. When your
energy runs low your window fills with static. Each start makes a new
level.

Keys
----
Two players (the default):
  Player 1:  W A S D drive, Ctrl fires
  Player 2:  the cursor keys drive, / (or keypad 0) fires
One player (TunnelTanks --single):
  the cursor keys drive, Ctrl fires
Esc quits.

A pad (GFOpenInput, or a joystick in port 2) also drives player 1: the
stick or d-pad drives, A or B fires, Start and Back together quit.

From the Shell
--------------
  TunnelTanks --single          one player
  TunnelTanks --level <GEN>     the level maker: toast (the default),
                                braid, maze or simple
  TunnelTanks --large           a far larger level
  TunnelTanks --seed <N>        the same level again for the same N
  TunnelTanks --help            all of them

What it needs
-------------
GFOS (AmigaOS 3.2 with GFOpenUp) on a 68040 or 68060; built soft float,
no FPU needed. The game draws on ACGame's 320 x 200 256-colour screen: on
RTG (and GFOpenGfx where it is installed) or AGA. It needs no SDL and no
GFOpenGPU. It ships acgame.library in the drawer's Libs/. Under 300 KB of
disk; the level takes about 2 MB of memory (a large one about 5 MB).

Sound: none. The game had none.

The game keeps nothing on disk.

Installing
----------
From the archive: double-click Install-TunnelTanks; it asks where the
drawer goes (Work:Games by default). With GFOpenUp:

  GFOpenUpTool INSTALL Parts/TunnelTanks

puts it in SYS:Games/TunnelTanks.

How it was ported
-----------------
TunnelTanks' author put everything SDL did behind one small layer, its
"gamelib" (src/include/gamelib.h), so the game could run without SDL (it
had an Android one too). The port is one more gamelib
(amiga/tt_gamelib.c), on ACGame (amiga/tt_acgame.c):

  - the game's 160 x 100 world is drawn two by two into one 320 x 200
    8-bit frame, which fills the screen exactly; each colour the game
    uses (about 40) gets a palette entry the first time it is drawn;
  - the frame is shown 24 times a second, as the game's own loop did;
  - the keys are the game's own (two players or one); the pad drives
    player 1, as upstream's joystick did;
  - the level pictures the game can write (--only-gen, --debug) are
    written as BMP files by the port.

The game's own code is unchanged apart from one line in main.c that
asked for SDL (amiga/patches/0001). On an emulated 68040 the game gives
the PC's frames byte for byte.

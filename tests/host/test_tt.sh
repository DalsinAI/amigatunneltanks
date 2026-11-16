#!/bin/sh
# Host tests: amiga/tt_gamelib.c's own pieces (tests/host/unit_test.c),
# then the whole game built for this PC from upstream's patched src/ and
# amiga/tt_gamelib.c, on tests/host/tt_host.c (a frame in memory, virtual
# time, scripted keys and pad), all with the same seed:
#  - start: two players' windows and the status bars, 24 frames a second;
#  - keys: tank 1 drives with D and fires with Ctrl, tank 2 with the cursor
#    keys and /: each window changes on its own;
#  - pad: the pad drives tank 1;
#  - single: one player's window (--single);
#  - quit: Esc ends the game;
#  - levels: each of upstream's level generators makes a level and plays;
#  - the same seed gives the same frames.
#   TT=/path/to/prepared (package/prepare-source.sh DIR) sh tests/host/test_tt.sh
#   TT_TEST_KEEP=DIR keeps the frames (PPM) there.
# Big-endian, as on the Amiga: with a 68k Linux cross compiler and QEMU,
#   HOSTCC="m68k-linux-gnu-gcc -static" TT_RUN="qemu-m68k -cpu m68040" sh tests/host/test_tt.sh
# runs the same tests on an emulated 68040; TT_SAME_AS=DIR (a PC run's
# TT_TEST_KEEP) then checks that every frame is byte for byte the PC's.
# GPL-3.0-or-later, like TunnelTanks. Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
TT=${TT:?TT=prepared source (package/prepare-source.sh DIR)}
CC=${HOSTCC:-cc}
RUN=${TT_RUN:-}
T=$(mktemp -d "${TMPDIR:-/tmp}/tt-host.XXXXXX")
trap 'rm -rf "$T"' EXIT INT TERM

INC="-I$TT/src/src/include -I$ROOT/amiga"
UP="-std=gnu99 -w -O1 $INC"
MINE="-std=gnu99 -Wall -Werror -O1 $INC"
mkdir -p "$T/up"
for f in "$TT"/src/src/*.c; do $CC $UP -c -o "$T/up/$(basename "$f" .c).o" "$f"; done
$CC $MINE -c -o "$T/tt_gamelib.o" "$ROOT/amiga/tt_gamelib.c"
$CC $MINE -c -o "$T/tt_host.o" "$HERE/tt_host.c"
$CC $MINE -c -o "$T/unit_test.o" "$HERE/unit_test.c"
$CC -o "$T/unit" "$T/unit_test.o" "$T/tt_gamelib.o" "$T/up/memalloc.o"
$CC -o "$T/tt" "$T"/up/*.o "$T/tt_gamelib.o" "$T/tt_host.o"

(cd "$T" && $RUN ./unit)

fail=0
summary() { sed -n "s/^$2 //p" "$1/summary.txt"; }
differ() { ! cmp -s "$1" "$2"; }
half_differs() {   # half_differs A.ppm B.ppm left|right: that half of the play area changed
    python3 -c 'import sys
def px(p):
    d=open(p,"rb").read(); return d[d.index(b"255\n")+4:]
a,b=px(sys.argv[1]),px(sys.argv[2]); x0=0 if sys.argv[3]=="left" else 160
sys.exit(0 if any(a[(y*320+x)*3:(y*320+x)*3+3]!=b[(y*320+x)*3:(y*320+x)*3+3] for y in range(170) for x in range(x0,x0+160)) else 1)' "$1" "$2" "$3"
}

run() {   # run NAME [ARGS...] with VAR=VALUE in the environment: the game until TT_END
    n=$1; shift
    mkdir -p "$T/$n"
    if ! (cd "$T/$n" && TT_OUT="$T/$n" $RUN "$T/tt" --seed 42 "$@" > stdout.txt 2> stderr.txt); then
        echo "FAIL $n: the game stopped"; tail -20 "$T/$n/stdout.txt" "$T/$n/stderr.txt"; fail=1; return 1
    fi
    [ -f "$T/$n/summary.txt" ] || { echo "FAIL $n: no summary"; fail=1; return 1; }
}

if TT_END=3000 TT_SHOTS=500,2900 run start; then
    [ "$(summary "$T/start" opened)" = 1 ] || { echo "FAIL start: the screen was not opened once"; fail=1; }
    s=$(summary "$T/start" shows); [ "$s" -ge 70 ] && [ "$s" -le 75 ] || { echo "FAIL start: $s frames in 3 s"; fail=1; }
    c=$(summary "$T/start" colours); [ "$c" -ge 8 ] || { echo "FAIL start: $c colours"; fail=1; }
    echo "start: two windows and the bars, $s frames in 3 s, $c colours"
fi

# tank 1: D (0x22) held, Ctrl (0x63) taps; tank 2: left (0x4F) held from 1.5 s, / (0x3A) taps
KEYS="500:34:1,1200:34:0,1300:99:1,1350:99:0,1500:79:1,2200:79:0,2300:58:1,2350:58:0"
if TT_END=2600 TT_KEYS="$KEYS" TT_SHOTS=450,1250,1450,2250 run keys; then
    half_differs "$T/keys/shot-450.ppm" "$T/keys/shot-1250.ppm" left || { echo "FAIL keys: tank 1 did not move"; fail=1; }
    half_differs "$T/keys/shot-450.ppm" "$T/keys/shot-1250.ppm" right && { echo "FAIL keys: tank 2 moved with tank 1's keys"; fail=1; }
    half_differs "$T/keys/shot-1450.ppm" "$T/keys/shot-2250.ppm" right || { echo "FAIL keys: tank 2 did not move"; fail=1; }
    echo "keys: tank 1 (D, Ctrl) and tank 2 (cursor keys, /) each drive their own window"
fi

if TT_END=1600 TT_PAD="500:8,1300:16,1400:0" TT_SHOTS=450,1250 run pad; then
    half_differs "$T/pad/shot-450.ppm" "$T/pad/shot-1250.ppm" left || { echo "FAIL pad: the pad did not drive tank 1"; fail=1; }
    echo "pad: the pad drives tank 1"
fi

if TT_END=1000 TT_SHOTS=500 run single --single; then
    differ "$T/start/shot-500.ppm" "$T/single/shot-500.ppm" || { echo "FAIL single: the same picture as two players"; fail=1; }
    echo "single: one player's window"
fi

if TT_END=30000 TT_KEYS="1000:69:1" run quit; then
    [ "$(summary "$T/quit" quit)" = 1 ] || { echo "FAIL quit: Esc did not end the game"; fail=1; }
    v=$(summary "$T/quit" vt); [ "$v" -lt 2000 ] || { echo "FAIL quit: ended at $v ms"; fail=1; }
    echo "quit: Esc ended the game at $v ms"
fi

for g in toast braid maze simple; do
    if TT_END=600 TT_SHOTS=500 run "level-$g" --level "$g"; then
        c=$(summary "$T/level-$g" colours); [ "$c" -ge 6 ] || { echo "FAIL level $g: $c colours"; fail=1; }
    fi
done
echo "levels: toast, braid, maze and simple each made and played"

if TT_END=3000 TT_SHOTS=500,2900 run again; then
    cmp -s "$T/start/shot-2900.ppm" "$T/again/shot-2900.ppm" || { echo "FAIL again: the same seed gave another frame"; fail=1; }
    echo "again: the same seed gives the same frames"
fi

SHOTS="start/shot-500 start/shot-2900 keys/shot-450 keys/shot-1250 keys/shot-1450 keys/shot-2250 pad/shot-1250 single/shot-500"
SHOTS="$SHOTS level-toast/shot-500 level-braid/shot-500 level-maze/shot-500 level-simple/shot-500"
if [ -n "${TT_SAME_AS:-}" ]; then
    n=0
    for f in $SHOTS; do
        n=$((n + 1))
        cmp -s "$T/$f.ppm" "$TT_SAME_AS/$f.ppm" || { echo "FAIL same: $f differs from $TT_SAME_AS"; fail=1; }
    done
    [ $fail = 0 ] && echo "same: all $n frames are byte for byte those of $TT_SAME_AS"
fi
if [ -n "${TT_TEST_KEEP:-}" ]; then
    mkdir -p "$TT_TEST_KEEP"
    for d in start keys pad single quit level-toast level-braid level-maze level-simple again; do cp -R "$T/$d" "$TT_TEST_KEEP/"; done
fi
[ $fail = 0 ] && echo "host tests: all passed"
exit $fail

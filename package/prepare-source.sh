#!/bin/sh
# The upstream source, ready to build: package/tt.lock's commit of
# raydog/tunneltanks is fetched into DIR/upstream (once), then
#   DIR/src   its src/ (the game and src/include), amiga/patches applied;
#             upstream's own gamelib backends (SDL, Android) are left out,
#             amiga/tt_gamelib.c takes their place;
#   DIR/LICENSE
# The game has no data files: its levels are generated.
#   sh package/prepare-source.sh DIR
# GPL-3.0-or-later, like TunnelTanks. Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/.." && pwd)
. "$HERE/tt.lock"
D=${1:?DIR}
mkdir -p "$D"
U="$D/upstream"
if [ "$(git -C "$U" rev-parse HEAD 2>/dev/null)" != "$TT_COMMIT" ]; then
    rm -rf "$U"; mkdir -p "$U"
    git -C "$U" init -q
    git -C "$U" fetch -q --depth 1 "$TT_GIT" "$TT_COMMIT"
    git -C "$U" checkout -q FETCH_HEAD
fi
[ "$(git -C "$U" rev-parse HEAD)" = "$TT_COMMIT" ] || { echo "tunneltanks: commit mismatch" >&2; exit 1; }

rm -rf "$D/src"
mkdir -p "$D/src/src/include"
cp "$U"/src/*.c "$D/src/src/"
cp "$U"/src/include/*.h "$D/src/src/include/"
for p in "$ROOT"/amiga/patches/*.patch; do
    patch -s -d "$D/src" -p1 < "$p"
done
cp "$U/LICENSE" "$D/LICENSE"

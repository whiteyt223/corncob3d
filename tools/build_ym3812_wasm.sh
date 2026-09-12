#!/usr/bin/env sh
# Build the BSD-3-Clause ymfm YM3812 bridge for an AudioWorklet.
set -eu
ZIG=${ZIG:-zig}
HERE=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
OBJDIR=${YMFM_BUILD_DIR:-/tmp/corncob-ymfm-wasm}
OUT=${1:-"$HERE/web/ym3812.wasm"}
mkdir -p "$OBJDIR"
for spec in 'core/ymfm_opl2_adapter.cpp adapter.o' 'third_party/ymfm/ymfm_opl.cpp opl.o' 'third_party/ymfm/ymfm_adpcm.cpp adpcm.o' 'third_party/ymfm/ymfm_pcm.cpp pcm.o'; do
    set -- $spec
    "$ZIG" c++ -target wasm32-wasi -Oz -fno-exceptions -fno-rtti -c -I"$HERE/third_party/ymfm" "$HERE/$1" -o "$OBJDIR/$2"
done
"$ZIG" c++ -target wasm32-wasi -Oz -fno-exceptions -fno-rtti \
    "$OBJDIR/adapter.o" "$OBJDIR/opl.o" "$OBJDIR/adpcm.o" "$OBJDIR/pcm.o" \
    -Wl,--no-entry -Wl,-z,stack-size=65536 \
    -Wl,--export=cc_ym3812_initialize -Wl,--export=cc_ym3812_reset \
    -Wl,--export=cc_ym3812_write -Wl,--export=cc_ym3812_native_rate \
    -Wl,--export=cc_ym3812_buffer -Wl,--export=cc_ym3812_render \
    -Wl,--export=cc_ym3812_sample_at -Wl,--export-memory \
    -Wl,--initial-memory=1048576 -Wl,--max-memory=1048576 -Wl,--strip-all -o "$OUT"

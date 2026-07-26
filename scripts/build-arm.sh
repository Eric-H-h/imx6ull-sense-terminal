#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
DAEMON_DIR="$ROOT_DIR/app/daemon"

TOOLCHAIN_BIN=${ARM_TOOLCHAIN_BIN:-"$HOME/.local/toolchains/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin"}
JPEG_ROOT=${ARM_JPEG_ROOT:-"$HOME/.local/sysroots/imx6ull-libjpeg-deb10-1.5.2-armhf"}
ARM_CC=${ARM_CC:-"$TOOLCHAIN_BIN/arm-linux-gnueabihf-gcc"}
ARM_READELF=${ARM_READELF:-"$TOOLCHAIN_BIN/arm-linux-gnueabihf-readelf"}
OUTPUT_DIR=${ARM_OUTPUT_DIR:-"$DAEMON_DIR/build/arm"}
OUTPUT="$OUTPUT_DIR/imx6ull-sense"

JPEG_INCLUDE="$JPEG_ROOT/usr/include"
JPEG_ARCH_INCLUDE="$JPEG_INCLUDE/arm-linux-gnueabihf"
JPEG_LIB="$JPEG_ROOT/usr/lib/arm-linux-gnueabihf"

require_executable()
{
    if [ ! -x "$1" ]; then
        echo "required executable not found: $1" >&2
        exit 2
    fi
}

require_file()
{
    if [ ! -f "$1" ]; then
        echo "required file not found: $1" >&2
        exit 2
    fi
}

cleanup_intermediate_files()
{
    make -C "$DAEMON_DIR" clean >/dev/null 2>&1 || true
}

require_executable "$ARM_CC"
require_executable "$ARM_READELF"
require_file "$JPEG_INCLUDE/jpeglib.h"
require_file "$JPEG_LIB/libjpeg.so"

mkdir -p "$OUTPUT_DIR"
trap cleanup_intermediate_files 0

make -C "$DAEMON_DIR" clean
make -C "$DAEMON_DIR" all \
    CC="$ARM_CC" \
    CPPFLAGS="-I$JPEG_INCLUDE -I$JPEG_ARCH_INCLUDE" \
    LDFLAGS="-L$JPEG_LIB -Wl,-rpath-link,$JPEG_LIB"

cp "$DAEMON_DIR/imx6ull-sense" "$OUTPUT"

echo "ARM output: $OUTPUT"
file "$OUTPUT"
"$ARM_READELF" -d "$OUTPUT" |
    grep -E 'NEEDED.*(libjpeg|libpthread|libc)'

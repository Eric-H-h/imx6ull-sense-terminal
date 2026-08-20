#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
INSTALLER="$ROOT_DIR/scripts/install-service.sh"
BINARY=${BINARY_SOURCE:-"$ROOT_DIR/app/daemon/build/arm/imx6ull-sense"}
CONFIG="$ROOT_DIR/config/config.json"
UNIT="$ROOT_DIR/systemd/imx6ull-sense.service"

require_file()
{
    if [ ! -f "$1" ]; then
        echo "required verification input not found: $1" >&2
        exit 2
    fi
}

require_unit_line()
{
    if ! grep -qx "$1" "$UNIT"; then
        echo "required unit setting missing: $1" >&2
        exit 1
    fi
}

require_file "$INSTALLER"
require_file "$BINARY"
require_file "$CONFIG"
require_file "$UNIT"

sh -n "$INSTALLER"
require_unit_line 'User=debian'
require_unit_line 'Group=debian'
require_unit_line 'Restart=on-failure'
require_unit_line 'RestartPreventExitStatus=78'
require_unit_line 'StartLimitIntervalSec=30'
require_unit_line 'StartLimitBurst=3'

if grep -qx 'Restart=always' "$UNIT"; then
    echo "unit must not use Restart=always" >&2
    exit 1
fi

TEMP_DIR=$(mktemp -d /tmp/imx6ull-sense-service-verify.XXXXXX)
cleanup()
{
    case "$TEMP_DIR" in
    /tmp/imx6ull-sense-service-verify.*) rm -rf "$TEMP_DIR" ;;
    esac
}
trap cleanup EXIT HUP INT TERM

PACKAGE_DIR="$TEMP_DIR/package"
DEST_ROOT="$TEMP_DIR/root"
mkdir -p "$PACKAGE_DIR"
cp "$BINARY" "$PACKAGE_DIR/imx6ull-sense"
cp "$CONFIG" "$PACKAGE_DIR/config.json"
cp "$UNIT" "$PACKAGE_DIR/imx6ull-sense.service"
cp "$INSTALLER" "$PACKAGE_DIR/install-service.sh"

DESTDIR="$DEST_ROOT" sh "$PACKAGE_DIR/install-service.sh"

test "$(stat -c '%a' "$DEST_ROOT/usr/local/bin/imx6ull-sense")" = 755
test "$(stat -c '%a' "$DEST_ROOT/etc/imx6ull-sense/config.json")" = 644
test "$(stat -c '%a' "$DEST_ROOT/etc/systemd/system/imx6ull-sense.service")" = 644
test "$(stat -c '%a' "$DEST_ROOT/var/lib/imx6ull-sense")" = 750

cmp "$BINARY" "$DEST_ROOT/usr/local/bin/imx6ull-sense"
cmp "$CONFIG" "$DEST_ROOT/etc/imx6ull-sense/config.json"
cmp "$UNIT" "$DEST_ROOT/etc/systemd/system/imx6ull-sense.service"

printf '%s\n' 'preserve-me' > "$DEST_ROOT/etc/imx6ull-sense/config.json"
DESTDIR="$DEST_ROOT" sh "$PACKAGE_DIR/install-service.sh" >/dev/null
test "$(cat "$DEST_ROOT/etc/imx6ull-sense/config.json")" = preserve-me

OVERWRITE_CONFIG=1 DESTDIR="$DEST_ROOT" \
    sh "$PACKAGE_DIR/install-service.sh" >/dev/null
cmp "$CONFIG" "$DEST_ROOT/etc/imx6ull-sense/config.json"

echo "service package verification: PASS"

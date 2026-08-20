#!/bin/sh
set -eu

PACKAGE_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
DESTDIR=${DESTDIR:-}
SERVICE_USER=debian
SERVICE_GROUP=debian
OVERWRITE_CONFIG=${OVERWRITE_CONFIG:-0}

BINARY_SOURCE=${BINARY_SOURCE:-"$PACKAGE_DIR/imx6ull-sense"}
CONFIG_SOURCE=${CONFIG_SOURCE:-"$PACKAGE_DIR/config.json"}
UNIT_SOURCE=${UNIT_SOURCE:-"$PACKAGE_DIR/imx6ull-sense.service"}

target_path()
{
    printf '%s%s\n' "$DESTDIR" "$1"
}

require_file()
{
    if [ ! -f "$1" ]; then
        echo "required package file not found: $1" >&2
        exit 2
    fi
}

require_file "$BINARY_SOURCE"
require_file "$CONFIG_SOURCE"
require_file "$UNIT_SOURCE"

if [ -z "$DESTDIR" ]; then
    if [ "$(id -u)" -ne 0 ]; then
        echo "run as root for a board install, or set DESTDIR for staging" >&2
        exit 2
    fi
    if ! getent passwd "$SERVICE_USER" >/dev/null 2>&1; then
        echo "service user not found: $SERVICE_USER" >&2
        exit 2
    fi
    if ! getent group "$SERVICE_GROUP" >/dev/null 2>&1; then
        echo "service group not found: $SERVICE_GROUP" >&2
        exit 2
    fi
fi

binary_target=$(target_path /usr/local/bin/imx6ull-sense)
config_dir=$(target_path /etc/imx6ull-sense)
config_target="$config_dir/config.json"
data_dir=$(target_path /var/lib/imx6ull-sense)
unit_target=$(target_path /etc/systemd/system/imx6ull-sense.service)

install -d -m 0755 "$(dirname -- "$binary_target")"
install -d -m 0755 "$config_dir"
install -d -m 0750 "$data_dir"
install -d -m 0755 "$(dirname -- "$unit_target")"

install -m 0755 "$BINARY_SOURCE" "$binary_target"
if [ ! -e "$config_target" ] || [ "$OVERWRITE_CONFIG" = 1 ]; then
    install -m 0644 "$CONFIG_SOURCE" "$config_target"
else
    echo "preserving existing config: $config_target"
fi
chmod 0644 "$config_target"
install -m 0644 "$UNIT_SOURCE" "$unit_target"

if [ -z "$DESTDIR" ]; then
    chown "$SERVICE_USER:$SERVICE_GROUP" "$data_dir"
    chown root:root "$binary_target" "$config_target" "$unit_target"
    systemctl daemon-reload
fi

echo "installed binary: $binary_target"
echo "installed config: $config_target"
echo "installed unit: $unit_target"
echo "service was not enabled or started"

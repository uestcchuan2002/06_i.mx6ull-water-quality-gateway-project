#!/bin/sh
set -e

SCRIPT_DIR=$(dirname "$0")

echo "=== Water Quality Gateway Installer ==="

if [ "$(id -u)" != "0" ]; then
    echo "ERROR: must run as root"
    exit 1
fi

echo "[1/5] Installing binary..."
if [ -f "$SCRIPT_DIR/../app/water_gateway" ]; then
    cp "$SCRIPT_DIR/../app/water_gateway" /usr/bin/water_gateway
elif [ -f "./water_gateway" ]; then
    cp ./water_gateway /usr/bin/water_gateway
else
    echo "ERROR: water_gateway binary not found"
    echo "  Place it in this directory or build with: cd app && make"
    exit 1
fi
chmod +x /usr/bin/water_gateway

echo "[2/5] Installing config..."
if [ -f "$SCRIPT_DIR/../config/gateway.conf" ]; then
    cp "$SCRIPT_DIR/../config/gateway.conf" /etc/water_gateway.conf
elif [ -f "./gateway.conf" ]; then
    cp ./gateway.conf /etc/water_gateway.conf
else
    echo "WARNING: gateway.conf not found, using defaults"
    touch /etc/water_gateway.conf
fi

echo "[3/5] Creating data directory..."
mkdir -p /var/lib/water_gateway

echo "[4/5] Installing init script..."
cp "$SCRIPT_DIR/water-gateway.sh" /etc/init.d/water_gateway
chmod +x /etc/init.d/water_gateway

echo "[5/5] Registering auto-start..."
if command -v update-rc.d >/dev/null 2>&1; then
    update-rc.d water_gateway defaults
elif command -v chkconfig >/dev/null 2>&1; then
    chkconfig --add water_gateway
    chkconfig water_gateway on
else
    echo "WARNING: update-rc.d / chkconfig not found"
    echo "  Manual registration:"
    echo "    ln -s /etc/init.d/water_gateway /etc/rc5.d/S99water_gateway"
    echo "    ln -s /etc/init.d/water_gateway /etc/rc0.d/K01water_gateway"
    echo "    ln -s /etc/init.d/water_gateway /etc/rc6.d/K01water_gateway"
fi

echo ""
echo "=== Installation complete ==="
echo "Usage:"
echo "  /etc/init.d/water_gateway start"
echo "  /etc/init.d/water_gateway stop"
echo "  /etc/init.d/water_gateway restart"
echo "  /etc/init.d/water_gateway status"
echo "  Logs: /var/log/water_gateway.log"

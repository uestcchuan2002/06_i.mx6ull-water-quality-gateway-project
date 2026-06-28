#!/bin/sh
### BEGIN INIT INFO
# Provides:          water_gateway
# Required-Start:    $network $local_fs
# Required-Stop:     $network $local_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Water Quality Linux Gateway
# Description:       Water Quality Monitoring Edge Gateway daemon
### END INIT INFO

DAEMON=/usr/bin/water_gateway
CONFIG=/etc/water_gateway.conf
PIDFILE=/var/run/water_gateway.pid
LOGDIR=/var/log

start() {
    if [ -f "$PIDFILE" ] && kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
        echo "water_gateway is already running (pid $(cat "$PIDFILE"))"
        return 1
    fi

    echo -n "Starting water_gateway: "
    if [ ! -x "$DAEMON" ]; then
        echo "FAILED ($DAEMON not found or not executable)"
        return 1
    fi

    $DAEMON -c "$CONFIG" >> "$LOGDIR/water_gateway.log" 2>&1 &
    echo $! > "$PIDFILE"
    echo "OK (pid $(cat "$PIDFILE"))"
    return 0
}

stop() {
    if [ ! -f "$PIDFILE" ] || ! kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
        echo "water_gateway is not running"
        rm -f "$PIDFILE"
        return 0
    fi

    PID=$(cat "$PIDFILE")
    echo -n "Stopping water_gateway (pid $PID): "
    kill "$PID"

    for i in 1 2 3 4 5 6 7 8 9 10; do
        if ! kill -0 "$PID" 2>/dev/null; then
            echo "OK (graceful exit)"
            rm -f "$PIDFILE"
            return 0
        fi
        sleep 1
    done

    echo -n "timed out, force killing: "
    kill -9 "$PID" 2>/dev/null
    rm -f "$PIDFILE"
    echo "OK"
    return 0
}

restart() {
    stop
    sleep 1
    start
}

status() {
    if [ -f "$PIDFILE" ] && kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
        echo "water_gateway is running (pid $(cat "$PIDFILE"))"
        return 0
    else
        echo "water_gateway is not running"
        [ -f "$PIDFILE" ] && rm -f "$PIDFILE"
        return 1
    fi
}

case "$1" in
    start)   start ;;
    stop)    stop ;;
    restart) restart ;;
    status)  status ;;
    *)
        echo "Usage: $0 {start|stop|restart|status}"
        exit 1
        ;;
esac

exit $?

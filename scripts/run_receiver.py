#!/usr/bin/env python3
"""
TCP receiver for water quality gateway upload test.
Keeps listening after gateway disconnects and reconnects.

Usage:
  python3 run_receiver.py [--host HOST] [--port PORT]

Default: listen on 0.0.0.0:18800
"""

import socket
import json
import argparse
import sys
import signal

def main():
    parser = argparse.ArgumentParser(description="Water Gateway upload receiver")
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=18800)
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind((args.host, args.port))
    sock.listen(1)
    print(f"[receiver] listening on {args.host}:{args.port}")

    shutdown = [False]

    def signal_handler(sig, frame):
        print(f"\n[receiver] stopping...")
        shutdown[0] = True
        sock.close()

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    total_count = 0
    total_alarms = 0

    while not shutdown[0]:
        try:
            conn, addr = sock.accept()
        except OSError:
            break

        print(f"[receiver] connected from {addr[0]}:{addr[1]}")

        session_count = 0
        session_alarms = 0
        buf = b""

        try:
            while not shutdown[0]:
                data = conn.recv(4096)
                if not data:
                    print(f"[receiver] connection closed by peer")
                    break

                buf += data
                while b'\n' in buf:
                    line, buf = buf.split(b'\n', 1)
                    if not line.strip():
                        continue
                    try:
                        msg = json.loads(line.decode('utf-8'))
                    except json.JSONDecodeError as e:
                        print(f"[receiver] json decode error: {e}")
                        continue

                    total_count += 1
                    session_count += 1
                    if msg.get('alarm_status', 0) != 0:
                        total_alarms += 1
                        session_alarms += 1

                    print(f"[#{total_count:04d}] device={msg['device_id']} "
                          f"ph={msg['ph']:.2f} temp={msg['temperature']:.2f} "
                          f"turb={msg['turbidity']:.2f} cond={msg['conductivity']} "
                          f"alarm={msg['alarm_status']} seq={msg['sequence']}")

        except socket.error as e:
            print(f"[receiver] socket error: {e}")
        finally:
            try:
                conn.close()
            except Exception:
                pass

        print(f"[receiver] session: {session_count}, alarms: {session_alarms} | "
              f"total: {total_count}, alarms: {total_alarms}")

    print(f"[receiver] done. total received: {total_count}, alarms: {total_alarms}")

if __name__ == "__main__":
    main()

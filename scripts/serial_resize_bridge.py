#!/usr/bin/env python3
"""Simple serial bridge that forwards stdin/stdout and sends terminal resize reports.

Usage:
    python serial_resize_bridge.py /dev/ttyACM0 [--baud 115200]

On every SIGWINCH (terminal resize) the script transmits CSI 8 ; rows ; cols t
so ecurses can resizeterm automatically.
"""

from __future__ import annotations

import argparse
import os
import selectors
import signal
import struct
import sys
import termios
import tty
from typing import Optional

try:
    import serial  # type: ignore
except ImportError as exc:  # pragma: no cover - import guard
    print("pyserial is required: pip install pyserial", file=sys.stderr)
    raise SystemExit(1) from exc

WINCH_SEQUENCE = "\x1b[8;{rows};{cols}t"


def get_window_size() -> Optional[tuple[int, int]]:
    try:
        packed = fcntl_ioctl(sys.stdout.fileno(), termios.TIOCGWINSZ, struct.pack("hhhh", 0, 0, 0, 0))
        rows, cols, _, _ = struct.unpack("hhhh", packed)
        return rows or 24, cols or 80
    except OSError:
        return None


def fcntl_ioctl(fd: int, op: int, data: bytes) -> bytes:
    import fcntl

    return fcntl.ioctl(fd, op, data)


def make_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Forward stdin/stdout to serial and send resize events")
    parser.add_argument("device", help="Serial device path, e.g. /dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    return parser


def set_terminal_raw(fd: int) -> termios.tcgetattr:
    old_settings = termios.tcgetattr(fd)
    tty.setraw(fd)
    return old_settings


def restore_terminal(fd: int, settings: termios.tcgetattr) -> None:
    termios.tcsetattr(fd, termios.TCSADRAIN, settings)


def send_resize(ser: serial.Serial) -> None:
    size = get_window_size()
    if not size:
        return
    rows, cols = size
    message = WINCH_SEQUENCE.format(rows=rows, cols=cols).encode("ascii")
    ser.write(message)
    ser.flush()


def main() -> int:
    parser = make_parser()
    args = parser.parse_args()

    try:
        ser = serial.Serial(args.device, args.baud, timeout=0)
    except serial.SerialException as exc:
        print(f"Failed to open {args.device}: {exc}", file=sys.stderr)
        return 1

    stdin_fd = sys.stdin.fileno()
    stdout_fd = sys.stdout.fileno()

    old_settings = set_terminal_raw(stdin_fd)

    selector = selectors.DefaultSelector()
    selector.register(stdin_fd, selectors.EVENT_READ)
    selector.register(ser, selectors.EVENT_READ)

    resize_pending = True

    def handle_winch(_sig, _frame):
        nonlocal resize_pending
        resize_pending = True

    signal.signal(signal.SIGWINCH, handle_winch)

    try:
        while True:
            if resize_pending:
                send_resize(ser)
                resize_pending = False

            events = selector.select(timeout=0.05)
            for key, _ in events:
                if key.fileobj is ser:
                    data = ser.read(1024)
                    if data:
                        os.write(stdout_fd, data)
                else:
                    data = os.read(stdin_fd, 1024)
                    if not data:
                        return 0
                    ser.write(data)
    except KeyboardInterrupt:
        return 0
    finally:
        restore_terminal(stdin_fd, old_settings)
        ser.close()


if __name__ == "__main__":
    sys.exit(main())

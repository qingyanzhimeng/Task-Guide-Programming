#!/usr/bin/env python3
"""
TCP echo server slow-reader / write-pressure test.

Usage:
    python3 echo_backpressure_test.py [HOST] [PORT]

Example:
    python3 echo_backpressure_test.py 127.0.0.1 8080

The test does not require a knowledge base. It expects the server to echo each
newline-terminated input line exactly, including the trailing newline.
"""

import socket
import sys
import threading
import time


HOST = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 8080

# 12 MB of echo data. Each line is <= 1024 bytes, matching the server's
# SINGLE_COMMAND_LENGTH limit (the body is exactly 1000 bytes).
LINE_COUNT = 12_000
BODY_LENGTH = 1000

CONNECT_TIMEOUT = 10
IO_TIMEOUT = 180


def make_payload():
    chunks = []
    for i in range(LINE_COUNT):
        prefix = f"{i:08d}:".encode("ascii")
        body = prefix + (b"x" * (BODY_LENGTH - len(prefix)))
        chunks.append(body + b"\n")
    payload = b"".join(chunks)
    return payload


def recv_exact(sock, expected_size, label):
    received = bytearray()
    last_report = 0

    while len(received) < expected_size:
        chunk = sock.recv(min(65536, expected_size - len(received)))
        if not chunk:
            raise RuntimeError(
                f"{label}: server closed early; "
                f"received {len(received)} / {expected_size} bytes"
            )
        received.extend(chunk)

        if len(received) - last_report >= 1024 * 1024:
            last_report = len(received)
            print(f"[{label}] received {len(received)} / {expected_size} bytes")

    return bytes(received)


def send_slow_client(payload, result):
    try:
        # This thread sends a large amount of data but does not read responses.
        slow_sock.sendall(payload)
        result["sent"] = True
        print(f"[A] finished sending {len(payload)} bytes")
    except Exception as exc:
        result["error"] = repr(exc)
        print(f"[A] sender error: {exc!r}")


def main():
    global slow_sock

    payload = make_payload()
    expected = payload

    print(f"[INFO] target: {HOST}:{PORT}")
    print(f"[INFO] commands: {LINE_COUNT}")
    print(f"[INFO] payload/expected echo: {len(payload)} bytes")

    # Client A: deliberately slow reader.
    slow_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    slow_sock.settimeout(IO_TIMEOUT)
    slow_sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4096)
    slow_sock.connect((HOST, PORT))
    print("[A] connected; receive buffer requested as 4096 bytes")

    sender_result = {}
    sender = threading.Thread(
        target=send_slow_client, args=(payload, sender_result), daemon=True
    )
    sender.start()

    # Give A time to fill the server-side send path while A itself does not read.
    time.sleep(2)

    # Client B: must still get a quick echo while A is not reading.
    fast_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    fast_sock.settimeout(10)
    fast_sock.connect((HOST, PORT))
    fast_message = b"FAST_CLIENT_CHECK\n"
    fast_sock.sendall(fast_message)

    fast_reply = recv_exact(fast_sock, len(fast_message), "B")
    if fast_reply != fast_message:
        raise RuntimeError(
            f"[B] wrong echo: expected {fast_message!r}, got {fast_reply!r}"
        )
    print("[PASS] client B received its echo while client A was not reading")
    fast_sock.close()

    # Now let A read. This should allow any pending server-side output to drain.
    print("[A] now starts reading the delayed echo stream")
    actual = recv_exact(slow_sock, len(expected), "A")

    sender.join(timeout=IO_TIMEOUT)
    if sender.is_alive():
        raise RuntimeError("[A] sender thread did not finish in time")
    if sender_result.get("error"):
        raise RuntimeError(f"[A] send failed: {sender_result['error']}")
    if not sender_result.get("sent"):
        raise RuntimeError("[A] sender did not confirm completion")

    if actual != expected:
        # Find the first differing byte to make debugging easier.
        limit = min(len(actual), len(expected))
        mismatch = next(
            (i for i in range(limit) if actual[i] != expected[i]), limit
        )
        raise RuntimeError(
            f"[A] echo mismatch at byte {mismatch}; "
            f"actual={len(actual)}, expected={len(expected)}"
        )

    print(f"[PASS] client A received all {len(actual)} bytes correctly")
    print("[PASS] no missing, duplicated, or reordered echo bytes detected")
    print()
    print("IMPORTANT:")
    print("This client-side test proves slow-reader isolation and data integrity.")
    print("It does NOT by itself prove that the server saw EAGAIN.")
    print("To prove EAGAIN, inspect/add a server log at the send() EAGAIN branch.")

    slow_sock.close()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n[STOP] interrupted by user")
        sys.exit(130)
    except Exception as exc:
        print(f"\n[FAIL] {exc}")
        sys.exit(1)

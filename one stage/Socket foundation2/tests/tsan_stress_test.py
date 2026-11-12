#!/usr/bin/env python3

import socket
import sys
import threading

HOST = "127.0.0.1"
DEFAULT_PORT = 8080

WORKERS = 8
ROUNDS_PER_WORKER = 50
SOCKET_TIMEOUT = 2.0
PAYLOAD = b"tsan-stress\n"

stats_lock = threading.Lock()

stats = {
    "attempts": 0,
    "connect_ok": 0,
    "echo_ok": 0,
    "bad_echo": 0,
    "errors": 0,
}

start_event = threading.Event()


def inc(key, amount=1):
    with stats_lock:
        stats[key] += amount


def recv_line(sock):
    data = bytearray()

    while b"\n" not in data:
        chunk = sock.recv(1024)

        if not chunk:
            return bytes(data)

        data.extend(chunk)

        if len(data) > 4096:
            return bytes(data)

    return bytes(data)


def worker(host, port):
    start_event.wait()

    for _ in range(ROUNDS_PER_WORKER):
        inc("attempts")

        sock = socket.socket(
            socket.AF_INET,
            socket.SOCK_STREAM
        )

        sock.settimeout(SOCKET_TIMEOUT)

        try:
            sock.connect((host, port))

            inc("connect_ok")

            sock.sendall(PAYLOAD)

            reply = recv_line(sock)

            if reply == PAYLOAD:
                inc("echo_ok")
            else:
                inc("bad_echo")

        except OSError:
            inc("errors")

        finally:
            try:
                sock.close()
            except OSError:
                pass


def main():
    port = DEFAULT_PORT
    host = HOST

    if len(sys.argv) >= 2:
        port = int(sys.argv[1])

    if len(sys.argv) >= 3:
        host = sys.argv[2]

    threads = []

    for _ in range(WORKERS):
        t = threading.Thread(
            target=worker,
            args=(host, port)
        )

        t.start()
        threads.append(t)

    total_expected = (
        WORKERS * ROUNDS_PER_WORKER
    )

    print(
        "[READY] workers=%d, rounds_per_worker=%d, total=%d"
        % (
            WORKERS,
            ROUNDS_PER_WORKER,
            total_expected
        )
    )

    start_event.set()

    for t in threads:
        t.join()

    print()
    print("===== TSan stress client summary =====")

    print(
        "attempts:   %d"
        % stats["attempts"]
    )

    print(
        "connect_ok: %d"
        % stats["connect_ok"]
    )

    print(
        "echo_ok:    %d"
        % stats["echo_ok"]
    )

    print(
        "bad_echo:   %d"
        % stats["bad_echo"]
    )

    print(
        "errors:     %d"
        % stats["errors"]
    )

    if (
        stats["attempts"] == total_expected
        and stats["connect_ok"] == total_expected
        and stats["echo_ok"] == total_expected
        and stats["bad_echo"] == 0
        and stats["errors"] == 0
    ):
        print()
        print(
            "[PASS] all %d connections "
            "completed correctly"
            % total_expected
        )

        return 0

    print()
    print(
        "[FAIL] some connections "
        "did not complete correctly"
    )

    return 1


if __name__ == "__main__":
    sys.exit(main())

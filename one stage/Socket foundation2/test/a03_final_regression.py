#!/usr/bin/env python3

from __future__ import print_function

import argparse
import os
import signal
import socket
import struct
import subprocess
import sys
import time


def recv_line(sock, timeout=2.0):
    sock.settimeout(timeout)
    data = b""

    while b"\n" not in data:
        chunk = sock.recv(4096)

        if not chunk:
            break

        data += chunk

    return data


def wait_until(predicate, timeout=2.0, interval=0.05):
    deadline = time.time() + timeout

    while time.time() < deadline:
        try:
            if predicate():
                return True
        except (OSError, IOError):
            return False

        time.sleep(interval)

    try:
        return bool(predicate())
    except (OSError, IOError):
        return False


def socket_fd_count(pid):
    fd_dir = "/proc/{}/fd".format(pid)
    count = 0

    for name in os.listdir(fd_dir):
        path = os.path.join(fd_dir, name)

        try:
            target = os.readlink(path)
        except OSError:
            continue

        if target.startswith("socket:["):
            count += 1

    return count


def port_is_available(port):
    probe = socket.socket(
        socket.AF_INET,
        socket.SOCK_STREAM
    )

    try:
        probe.setsockopt(
            socket.SOL_SOCKET,
            socket.SO_REUSEADDR,
            1
        )

        probe.bind(
            ("0.0.0.0", port)
        )

        return True

    except OSError:
        return False

    finally:
        probe.close()


def choose_ports(base_port):
    if base_port != 0:
        ports = [
            base_port + i
            for i in range(5)
        ]

        for port in ports:
            if port < 1024 or port > 49151:
                raise ValueError(
                    "base port must leave five ports "
                    "inside 1024..49151"
                )

        return ports

    base = 18080

    while base <= 49147:

        ports = [
            base + i
            for i in range(5)
        ]

        if all(
            port_is_available(port)
            for port in ports
        ):
            return ports

        base += 5

    raise RuntimeError(
        "unable to find five free TCP ports"
    )


def wait_server(
    host,
    port,
    process,
    timeout=3.0
):
    deadline = time.time() + timeout

    while time.time() < deadline:

        if process.poll() is not None:
            return False

        probe = socket.socket(
            socket.AF_INET,
            socket.SOCK_STREAM
        )

        probe.settimeout(0.15)

        try:
            probe.connect(
                (host, port)
            )

            probe.close()

            return True

        except OSError:
            probe.close()

            time.sleep(0.05)

    return False


def drain_process_output(process):
    if process.stdout is None:
        return ""

    try:
        return process.stdout.read()

    except Exception:
        return ""


def stop_process(process):
    if process.poll() is None:

        try:
            process.send_signal(
                signal.SIGINT
            )

            process.wait(
                timeout=1.5
            )

        except subprocess.TimeoutExpired:

            process.kill()

            process.wait(
                timeout=1.0
            )

        except OSError:
            pass

    return drain_process_output(
        process
    )


def start_server(
    server_path,
    host,
    port
):
    process = subprocess.Popen(
        [
            server_path,
            str(port)
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
        bufsize=1
    )

    if not wait_server(
        host,
        port,
        process
    ):

        if process.poll() is None:
            output = stop_process(
                process
            )
        else:
            output = drain_process_output(
                process
            )

        raise RuntimeError(
            "server failed to listen on "
            "{}:{}\n{}".format(
                host,
                port,
                output
            )
        )

    cleaned = wait_until(
        lambda:
            socket_fd_count(
                process.pid
            ) == 1,
        timeout=2.0
    )

    if not cleaned:

        output = stop_process(
            process
        )

        raise RuntimeError(
            "server did not return to one "
            "listening socket after probe\n"
            + output
        )

    return process


def test_echo(
    server,
    host,
    port
):
    process = start_server(
        server,
        host,
        port
    )

    try:
        sock = socket.create_connection(
            (host, port),
            timeout=2.0
        )

        try:
            messages = [
                "hello",
                "china",
                "晚安",
                "socket"
            ]

            for message in messages:

                payload = (
                    message + "\n"
                ).encode("utf-8")

                sock.sendall(
                    payload
                )

                reply = recv_line(
                    sock
                )

                if reply != payload:

                    return (
                        False,
                        "{}: expected={!r}, "
                        "got={!r}".format(
                            repr(message),
                            payload,
                            reply
                        )
                    )

        finally:
            sock.close()

        if process.poll() is not None:

            return (
                False,
                "server exited unexpectedly"
            )

        return (
            True,
            "same TCP connection completed "
            "multiple Echo exchanges"
        )

    finally:
        stop_process(
            process
        )


def test_normal_close(
    server,
    host,
    port
):
    process = start_server(
        server,
        host,
        port
    )

    try:
        baseline = socket_fd_count(
            process.pid
        )

        sock = socket.create_connection(
            (host, port),
            timeout=2.0
        )

        connected = wait_until(
            lambda:
                socket_fd_count(
                    process.pid
                ) >= baseline + 1,
            timeout=2.0
        )

        sock.sendall(
            b"hello\n"
        )

        reply = recv_line(
            sock
        )

        if reply != b"hello\n":

            sock.close()

            return (
                False,
                "Echo before close failed: "
                "{!r}".format(reply)
            )

        connected_count = (
            socket_fd_count(
                process.pid
            )
        )

        sock.close()

        released = wait_until(
            lambda:
                socket_fd_count(
                    process.pid
                ) == baseline,
            timeout=2.0
        )

        after_count = (
            socket_fd_count(
                process.pid
            )
        )

        server_alive = (
            process.poll() is None
        )

        ok = (
            connected
            and released
            and server_alive
        )

        return (
            ok,
            "socket_fd_count: "
            "baseline={}, "
            "connected={}, "
            "after_close={}; "
            "server_alive={}".format(
                baseline,
                connected_count,
                after_count,
                server_alive
            )
        )

    finally:
        stop_process(
            process
        )


def test_rst(
    server,
    host,
    port
):
    process = start_server(
        server,
        host,
        port
    )

    try:
        baseline = socket_fd_count(
            process.pid
        )

        sock = socket.create_connection(
            (host, port),
            timeout=2.0
        )

        sock.sendall(
            b"hello\n"
        )

        reply = recv_line(
            sock
        )

        if reply != b"hello\n":

            sock.close()

            return (
                False,
                "Echo before RST failed: "
                "{!r}".format(reply)
            )

        linger = struct.pack(
            "ii",
            1,
            0
        )

        sock.setsockopt(
            socket.SOL_SOCKET,
            socket.SO_LINGER,
            linger
        )

        sock.close()

        released = wait_until(
            lambda:
                socket_fd_count(
                    process.pid
                ) == baseline,
            timeout=2.0
        )

        server_alive = (
            process.poll() is None
        )

        return (
            released and server_alive,
            "fd_released={}; "
            "server_alive={}".format(
                released,
                server_alive
            )
        )

    finally:

        output = stop_process(
            process
        )

        interesting = []

        for line in output.splitlines():

            lower = line.lower()

            if (
                "errno:" in lower
                or
                "reset" in lower
            ):
                interesting.append(
                    line
                )

        if interesting:

            print(
                "    server log: "
                + " | ".join(
                    interesting[-3:]
                )
            )


def test_server_exit(
    server,
    client,
    host,
    port
):
    server_process = start_server(
        server,
        host,
        port
    )

    baseline = socket_fd_count(
        server_process.pid
    )

    client_process = subprocess.Popen(
        [
            client,
            host,
            str(port)
        ],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
        bufsize=1
    )

    try:
        connected = wait_until(
            lambda: (
                client_process.poll()
                is None
                and
                socket_fd_count(
                    server_process.pid
                ) >= baseline + 1
            ),
            timeout=2.0
        )

        if not connected:

            return (
                False,
                "tcp_client did not "
                "establish a connection"
            )

        if client_process.stdin is None:

            return (
                False,
                "tcp_client stdin pipe unavailable"
            )

        client_process.stdin.write(
            "hello\n"
        )

        client_process.stdin.flush()

        time.sleep(0.6)

        stop_process(
            server_process
        )

        time.sleep(0.3)

        try:

            client_process.stdin.write(
                "again\n"
            )

            client_process.stdin.flush()

        except (
            BrokenPipeError,
            OSError
        ):
            pass

        try:
            client_process.stdin.close()

        except OSError:
            pass

        client_process.stdin = None

        try:

            output, _ = (
                client_process.communicate(
                    timeout=3.0
                )
            )

        except subprocess.TimeoutExpired:

            client_process.kill()

            output, _ = (
                client_process.communicate()
            )

            return (
                False,
                "client hung after server exit\n"
                + output[-1000:]
            )

        return_code = (
            client_process.returncode
        )

        sigpipe_killed = (
            return_code
            == -signal.SIGPIPE
        )

        ok = (
            not sigpipe_killed
        )

        return (
            ok,
            "client_returncode={}; "
            "SIGPIPE_killed={}\n{}".format(
                return_code,
                sigpipe_killed,
                output[-1000:]
            )
        )

    finally:

        if (
            server_process.poll()
            is None
        ):
            stop_process(
                server_process
            )

        if (
            client_process.poll()
            is None
        ):
            client_process.kill()

            client_process.wait(
                timeout=1.0
            )


def test_port_in_use(
    server,
    host,
    port
):
    first_server = start_server(
        server,
        host,
        port
    )

    try:

        try:

            second = subprocess.run(
                [
                    server,
                    str(port)
                ],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                universal_newlines=True,
                timeout=3.0
            )

        except subprocess.TimeoutExpired:

            return (
                False,
                "second server did not exit "
                "after bind failure"
            )

        output = (
            second.stdout
        )

        first_alive = (
            first_server.poll()
            is None
        )

        recognized = (
            "Address already in use"
            in output
            or
            "The port has already been used"
            in output
        )

        ok = (
            first_alive
            and recognized
        )

        return (
            ok,
            "first_server_alive={}\n"
            "second_server_returncode={}\n"
            "second_server_output:\n{}".format(
                first_alive,
                second.returncode,
                output.strip()
            )
        )

    finally:

        stop_process(
            first_server
        )


def main():

    parser = argparse.ArgumentParser(
        description=(
            "A03 TCP final regression test "
            "(Python 3.6 compatible)"
        )
    )

    parser.add_argument(
        "--server",
        default="./server",
        help="path to the server executable"
    )

    parser.add_argument(
        "--client",
        default="./tcp_client/tcp_client",
        help="path to the tcp_client executable"
    )

    parser.add_argument(
        "--host",
        default="127.0.0.1",
        help="server address used by the tests"
    )

    parser.add_argument(
        "--port",
        type=int,
        default=0,
        help=(
            "base port; 0 automatically "
            "chooses five free ports"
        )
    )

    args = parser.parse_args()

    for executable in (
        args.server,
        args.client
    ):

        if not os.path.isfile(
            executable
        ):

            print(
                "ERROR: executable not found: "
                "{}".format(executable)
            )

            return 2

        if not os.access(
            executable,
            os.X_OK
        ):

            print(
                "ERROR: file is not executable: "
                "{}".format(executable)
            )

            return 2

    try:
        ports = choose_ports(
            args.port
        )

    except Exception as exc:

        print(
            "ERROR: cannot choose test ports: "
            "{}".format(exc)
        )

        return 2

    tests = [

        (
            "1. normal Echo / multi-round",

            lambda:
                test_echo(
                    args.server,
                    args.host,
                    ports[0]
                )
        ),

        (
            "2. orderly close / fd release",

            lambda:
                test_normal_close(
                    args.server,
                    args.host,
                    ports[1]
                )
        ),

        (
            "3. RST / fd release",

            lambda:
                test_rst(
                    args.server,
                    args.host,
                    ports[2]
                )
        ),

        (
            "4. server exit / SIGPIPE protection",

            lambda:
                test_server_exit(
                    args.server,
                    args.client,
                    args.host,
                    ports[3]
                )
        ),

        (
            "5. EADDRINUSE",

            lambda:
                test_port_in_use(
                    args.server,
                    args.host,
                    ports[4]
                )
        )
    ]

    print(
        "server: {}".format(
            args.server
        )
    )

    print(
        "client: {}".format(
            args.client
        )
    )

    print(
        "host:   {}".format(
            args.host
        )
    )

    print(
        "ports:  {}".format(
            ports
        )
    )

    print()

    failures = 0

    for name, test in tests:

        try:
            ok, detail = test()

        except Exception as exc:

            ok = False

            detail = (
                "unexpected exception: "
                "{}: {}".format(
                    type(exc).__name__,
                    exc
                )
            )

        if ok:

            print(
                "PASS {}".format(
                    name
                )
            )

        else:

            print(
                "FAIL {}".format(
                    name
                )
            )

            failures += 1

        for line in str(
            detail
        ).splitlines():

            print(
                "    {}".format(
                    line
                )
            )

        print()

    passed = (
        len(tests)
        - failures
    )

    print(
        "SUMMARY: {}/{} passed".format(
            passed,
            len(tests)
        )
    )

    if failures == 0:

        print(
            "A03 final regression: PASS"
        )

        return 0

    print(
        "A03 final regression: FAIL"
    )

    return 1


if __name__ == "__main__":
    sys.exit(
        main()
    )

from __future__ import print_function

import socket
import threading

HOST = "127.0.0.1"
PORT = 8080

CLIENTS = 8
ROUNDS = 50

start = threading.Event()
errors = []
errors_lock = threading.Lock()


def record_error(item):
    with errors_lock:
        errors.append(item)


def worker(index):
    start.wait()

    for round_id in range(ROUNDS):
        sock = socket.socket(
            socket.AF_INET,
            socket.SOCK_STREAM
        )

        sock.settimeout(2.0)

        try:
            sock.connect(
                (HOST, PORT)
            )

            message = (
                "client-{}-{}\n".format(
                    index,
                    round_id
                )
            ).encode("utf-8")

            sock.sendall(message)

            data = b""

            while b"\n" not in data:
                chunk = sock.recv(4096)

                if not chunk:
                    break

                data += chunk

            if data != message:
                record_error(
                    (
                        index,
                        round_id,
                        "echo mismatch",
                        repr(data)
                    )
                )

        except Exception as exc:
            record_error(
                (
                    index,
                    round_id,
                    repr(exc)
                )
            )

        finally:
            sock.close()


threads = []

for i in range(CLIENTS):
    thread = threading.Thread(
        target=worker,
        args=(i,)
    )

    threads.append(thread)
    thread.start()


start.set()


for thread in threads:
    thread.join()


print(
    "stress finished, clients={}, rounds={}, total_connections={}, errors={}".format(
        CLIENTS,
        ROUNDS,
        CLIENTS * ROUNDS,
        len(errors)
    )
)

if errors:
    print("first errors:")
    print(errors[:10])

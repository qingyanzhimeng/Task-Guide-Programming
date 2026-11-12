#!/usr/bin/env python3

import socket
import sys
import threading
import time


HOST = "127.0.0.1"
DEFAULT_PORT = 8080

# 同时进行连接尝试的客户端线程数量
WORKERS = 4

# 整个测试最多持续的时间
TEST_DURATION = 6.0

# connect / recv / send 的超时
CONNECT_TIMEOUT = 0.5
IO_TIMEOUT = 0.5

# 稍微限制连接速度，避免这次实验变成线程资源压力测试
PAUSE_BETWEEN_ATTEMPTS = 0.05

# 连续多次 Connection refused 后，
# 认为服务器已经真正停止监听
REFUSED_LIMIT = 3

# 你的当前服务器是 Echo Server，
# 所以发送一行，再检查是否原样返回
PAYLOAD = b"race-test\n"


stats_lock = threading.Lock()

stats = {
    "attempts": 0,
    "connect_ok": 0,
    "echo_ok": 0,
    "peer_closed": 0,
    "refused": 0,
    "reset": 0,
    "broken_pipe": 0,
    "timeout": 0,
    "bad_echo": 0,
    "other_os_error": 0,
}


# 让所有测试线程准备好之后一起开始
start_event = threading.Event()


def inc(key, amount=1):
    """
    多个测试线程都会修改 stats，
    所以使用互斥锁保护统计数据。
    """
    with stats_lock:
        stats[key] += amount


def recv_line(sock):
    """
    接收一整行数据。

    返回：
        (收到的数据, 对端是否在收到数据前保持连接)
    """

    data = bytearray()

    while b"\n" not in data:

        chunk = sock.recv(1024)

        # recv 返回空字节串：
        # 对端已经关闭连接
        if not chunk:
            return bytes(data), False

        data.extend(chunk)

        # 防止测试程序本身因为异常服务器无限积累数据
        if len(data) > 4096:
            return bytes(data), True

    return bytes(data), True


def worker(worker_id, host, port, deadline):

    # 所有 worker 同时开始
    start_event.wait()

    consecutive_refused = 0

    while time.monotonic() < deadline:

        inc("attempts")

        sock = socket.socket(
            socket.AF_INET,
            socket.SOCK_STREAM
        )

        sock.settimeout(CONNECT_TIMEOUT)

        try:
            # -------------------------------------------------
            # 1. 不断建立新的 TCP 连接
            # -------------------------------------------------
            sock.connect((host, port))

            inc("connect_ok")

            consecutive_refused = 0

            # -------------------------------------------------
            # 2. 建立成功后发送一条 Echo 消息
            # -------------------------------------------------
            sock.settimeout(IO_TIMEOUT)

            sock.sendall(PAYLOAD)

            # -------------------------------------------------
            # 3. 等服务器回复
            # -------------------------------------------------
            reply, peer_still_open = recv_line(sock)

            if not peer_still_open:
                # stop_server() 可能正在 shutdown 这个连接
                inc("peer_closed")

            elif reply == PAYLOAD:
                inc("echo_ok")

            else:
                inc("bad_echo")

        except ConnectionRefusedError:
            # listener 已经关闭后，
            # 后续 connect 很可能来到这里
            inc("refused")

            consecutive_refused += 1

            if consecutive_refused >= REFUSED_LIMIT:
                break

        except ConnectionResetError:
            # stop_server 与客户端通信发生竞争时可能观察到
            inc("reset")

        except BrokenPipeError:
            # 连接已经停止发送
            inc("broken_pipe")

        except socket.timeout:
            inc("timeout")

        except OSError:
            # 其它 socket 系统错误
            inc("other_os_error")

        finally:
            try:
                sock.close()
            except OSError:
                pass

        # 控制连接速度
        time.sleep(PAUSE_BETWEEN_ATTEMPTS)


def main():

    host = HOST
    port = DEFAULT_PORT

    # 用法：
    #
    # python3 race_connect_test.py 8080
    #
    # 或：
    #
    # python3 race_connect_test.py 8080 127.0.0.1

    if len(sys.argv) >= 2:
        port = int(sys.argv[1])

    if len(sys.argv) >= 3:
        host = sys.argv[2]

    deadline = time.monotonic() + TEST_DURATION

    threads = []

    # ---------------------------------------------------------
    # 创建多个并发连接线程
    # ---------------------------------------------------------
    for i in range(WORKERS):

        t = threading.Thread(
            target=worker,
            args=(i, host, port, deadline)
        )

        t.start()

        threads.append(t)

    print(
        "[READY] %d connector threads are ready for %s:%d"
        % (WORKERS, host, port)
    )

    print(
        "[ACTION] While this script is connecting, "
        "type quit in the server terminal."
    )

    # 一起开始
    start_event.set()

    # ---------------------------------------------------------
    # 等待所有测试线程结束
    # ---------------------------------------------------------
    for t in threads:
        t.join()

    # ---------------------------------------------------------
    # 输出结果
    # ---------------------------------------------------------
    print()
    print("===== client-side summary =====")

    ordered_keys = [
        "attempts",
        "connect_ok",
        "echo_ok",
        "peer_closed",
        "refused",
        "reset",
        "broken_pipe",
        "timeout",
        "bad_echo",
        "other_os_error",
    ]

    for key in ordered_keys:
        print(
            "%-16s %d"
            % (key + ":", stats[key])
        )

    # 是否观察到了服务器停机产生的影响
    stop_observed = (
        stats["peer_closed"]
        + stats["refused"]
        + stats["reset"]
        + stats["broken_pipe"]
        + stats["timeout"]
    ) > 0

    # ---------------------------------------------------------
    # 客户端侧初步判断
    # ---------------------------------------------------------
    if (
        stats["connect_ok"] > 0
        and stop_observed
    ):
        print()
        print(
            "[CLIENT OBSERVATION PASS] "
            "The client side saw both successful "
            "connections and shutdown effects."
        )

        print(
            "[MANUAL CHECK REQUIRED] "
            "The real pass condition is that the server "
            "also exits cleanly without hanging or terminating."
        )

        return 0

    if stats["connect_ok"] == 0:

        print()
        print(
            "[INCONCLUSIVE] No connection succeeded. "
            "Check host/port and start the server first."
        )

        return 2

    print()
    print(
        "[INCONCLUSIVE] The script did not observe "
        "the server stopping during this run."
    )

    print(
        "Run it again and type quit while "
        "connection attempts are active."
    )

    return 3


if __name__ == "__main__":
    sys.exit(main())

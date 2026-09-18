# 2026-09-18：多进程 TCP 文件传输

沿用当天 `20260918` 目录，将两个 Linux/Socket 对话中的多进程与文件传输练习整理在一起。保留 `socket_client` / `socket_server` 类、原有函数命名和 POSIX 文件操作风格。

| 文件 | 内容 |
|---|---|
| [client_file.cpp](client_file.cpp) | 参数解析、文件信息、等待 ACK、分块发送 |
| [server_file.cpp](server_file.cpp) | accept/fork、文件信息接收、ACK、分块保存 |
| [tests/test_transfer.py](tests/test_transfer.py) | 临时目录编译与真实 Socket 回归测试 |
| [aaa.txt](aaa.txt)、[recv_aaa.txt](recv_aaa.txt) | 原有文本传输样例 |

目录中原有 `client` / `server` 是历史编译产物，未重建；运行当前版本应按下方命令重新编译。

## 编译与运行

在仓库根目录执行（Linux，C++11）：

```sh
build_dir=$(mktemp -d)
g++ -std=c++11 -Wall -Wextra -Werror 20260918/server_file.cpp -o "$build_dir/server"
g++ -std=c++11 -Wall -Wextra -Werror 20260918/client_file.cpp -o "$build_dir/client"
```

终端一：进入用于接收文件的目录，使用上一步输出路径运行 `server`。服务器沿用 `0.0.0.0:8080`，按 Ctrl+C 停止。

```sh
mkdir -p "$build_dir/received"
(cd "$build_dir/received" && ../server)
```

终端二：将 `build_dir` 设为终端一创建的实际路径，然后从仓库根目录发送：

```sh
"$build_dir/client" 127.0.0.1 8080 20260918/aaa.txt
cmp 20260918/aaa.txt "$build_dir/received/recv_aaa.txt"
sha256sum 20260918/aaa.txt "$build_dir/received/recv_aaa.txt"
```

跨机器时将 `127.0.0.1` 换为服务器 IPv4 地址。`recv_aaa.txt` 位于**服务器进程的当前工作目录**。客户端支持路径参数，但元信息中只发送文件名。

## 1. server_sock / client_sock 与 fork

`server_sock` 依次用于 `socket → bind → listen → accept`，负责监听新连接。`accept()` 返回的 `client_sock` 是服务端与本次客户端通信的 fd；不会替换监听 fd。

```text
父进程 accept(server_sock)
           ↓ 得到 client_sock
         fork()
         ├─ 父进程：close(client_sock) → 继续 accept
         └─ 子进程：close(server_sock) → 处理文件
                                      → close(client_sock) → _exit(0)
```

`fork()` 复制 fd 表，父子进程中的对应 fd 引用同一个内核 Socket。`close()` 只释放当前进程持有的引用，不会直接关闭另一进程仍持有的 fd。父进程若保留连接 fd，子进程结束后连接仍被引用；子进程若保留监听 fd，也会延长监听 Socket 生命周期。

处理结束必须退出子进程，否则会返回循环，对已关闭的 `server_sock` 再次 `accept()`，出现 `Accept failed`。失败路径同样关闭连接并 `_exit(1)`；`fork()` 失败时父进程关闭刚接受的连接。当前用 `SIGCHLD = SIG_IGN` 自动回收子进程；后续可练习 `SIGCHLD + waitpid(-1, ..., WNOHANG)`，不要在父进程每次 fork 后阻塞等待而破坏并发。

## 2. argc / argv、stoi、htons

```text
./client 127.0.0.1 8080 aaa.txt
argv[0]   argv[1] argv[2] argv[3]     argc == 4
```

先检查 `argc`，再读取 `argv[1..3]`。`stoi(argv[2], &used)` 把端口文本转为整数，检查是否消费整个参数且范围在 1～65535，然后 `htons(port)` 转为 16 位网络字节序。IP 使用 `inet_pton(AF_INET, ...)` 转换。

`stoi` 无法转换时抛 `invalid_argument`，越界时抛 `out_of_range`；缺少参数时越界访问 `argv` 本身就不合法，不能把所有崩溃都简单归因于 `stoi`。

## 3. 协议与完整读写

两端保持一致的结构：

```cpp
struct file_Info
{
    char file_name[256];
    uint64_t file_size;
};
```

```text
Client                                  Server 子进程
stat → file_Info → send_all ───────────→ recv_all(sizeof(file_Info))
                                         检查文件名
recv_all(2) / 校验 "OK" ←────────────── send_all("OK", 2)
open → read → send_all ────────────────→ recv_file → open → recv → write
                                         按 file_size 收满后关闭
```

ACK 只包含两个字节 `O`、`K`，不发送结尾 `\0`。客户端用 `char ack[3] = {0}` 并累计接收两字节，再比较；一次 `read()` 不保证能读全 ACK。

`send_all()` / `recv_all()` 用 `ptr + total` 与 `len - total` 处理短写/短读，按系统调用实际返回值累计，遇到 `EINTR` 重试。`send/recv/read/write` 的结果使用 **`ssize_t`**，因为错误返回 `-1`；累计长度用 `size_t`。如果把 `send()` 结果存到无符号 `size_t`，`-1` 会变成很大的正数，错误判断和偏移计算都会出问题。

客户端收到 OK 后 `open(argv[3], O_RDONLY)`，每次 `read()` 最多 4096 字节，只把本次返回的 `n` 字节交给 `send_all()`。服务端每次接收 `min(4096, file_size - total_received)` 字节，再循环 `write()`，直至这一块完整写入。不能假设磁盘 `write()` 一次总能写完。

## 4. 二进制文件与 TCP 字节流

TCP 没有“文本发送模式”和“二进制发送模式”，也不保留一次 `send()` 的消息边界。`open/read/send` 已经传输原始字节，可用于文本、图片、PDF 等。正文中可以包含 `0x00`，所以不能用 `strlen(buffer)` 决定正文长度；也不能总是发送 `sizeof(buffer)`，否则最后一块可能附带无效字节。

`ifstream(..., ios::binary)` 是另一种读文件方式，最后仍然可以调用 `send()`。本练习保留 Linux/POSIX fd 操作，以串联磁盘 fd 和 Socket fd；无需把原始字节转换为字符 `0`、`1`。

## 5. 今日调试记录

| 现象 | 原因与处理 |
|---|---|
| `stoi invalid_argument` / 缺少参数时异常 | 先检查 argc；捕获转换异常，拒绝非法、越界或带尾随内容的端口 |
| `bind: Address already in use` | 用 `perror("bind")` 查看真实错误；Linux 下 `ss -ltnp 'sport = :8080'` 确认占用者；确认后停止自己的旧服务 |
| 重启时地址复用 | 在 bind 前设置 `SO_REUSEADDR`；它不允许抢占仍在监听的服务 |
| `open()` 不能直接接受 `std::string` | 使用 `recv_name.c_str()` 提供 C 字符串；指针有效期依赖原 string |
| 发完元信息但没有生成文件 | 定义 `recv_file()` 不等于执行，服务端发 ACK 后必须实际调用，并检查返回值 |
| 客户端只收到 `O` 就结束 | TCP 可拆分 ACK，改为 `recv_all(..., 2)` |
| 子进程文件处理后出现 `Accept failed` | 处理后 `close(client_sock) + _exit(0)`，失败用 `_exit(1)` |
| `send()` 失败未被识别 | 返回值从 `size_t` 改为 `ssize_t` |

本次本机编译还遇到 `using namespace std` 下 `bind` 与标准库名称冲突，使用 `::bind` 明确调用系统函数。

## 验证与当前边界

运行自动验证：

```sh
python3 20260918/tests/test_transfer.py
```

需要 C++ 编译器和 Python 3，8080 必须空闲。测试在临时目录编译、收发，不覆盖仓库中的历史产物。可通过 `CXX=g++` 指定编译器。

2026-09-18 本次在 **macOS / clang++** 上以 `-std=c++11 -Wall -Wextra -Werror` 编译通过，8 项真实 Socket 回归测试通过：参数缺失、非法参数/文件不存在、拆分 ACK、文本/大于 1 MiB 的二进制/空文件及路径、拆分元信息和正文、慢连接下另一个客户端完成、非法文件名、中途断开。收发内容按字节比对。此次结果不等同于重新在 Linux 上验证。

当前仍是学习版本：

- 直接发送结构体内存，`file_size` 没有单独序列化或转网络字节序；要求两端结构布局、整数表示和字节序兼容，不是跨平台通用协议。`htons` 仅处理端口。
- OK 表示元信息已接收，不代表目标文件已打开或最终保存成功。客户端发送成功仅说明字节已交给本端内核；最终应查看服务端日志并用 `cmp` / 哈希验证。尚无文件完成后的最终 ACK。
- `recv_` 前缀保护普通原文件名，但 `O_TRUNC` 会覆盖同名的已有接收文件。并发测试使用不同文件名；同名并发传输、符号链接、原子替换尚未处理。
- 接收失败可能留下不完整的 `recv_` 文件；发送期间应保持源文件不变。尚无超时、断点续传或文件大小上限，仅用于可信实验环境。
- 文件名字段保留 256 字节，当前限制实际名称 1～250 字节，为常见文件系统的 `recv_` 前缀预留空间；服务端检查终止符和路径分隔符。

对应对话：创新文章推荐方案（`6aac8e81-2ac0-83ee-8a08-41b4225b0f79`）、继续Linux学习（`6aab5bd8-d608-83e9-ad04-a101149672e7`）。

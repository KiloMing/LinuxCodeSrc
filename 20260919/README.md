# 2026-09-19 Select Server

## Files

- `select_server.cpp`: learning version of a multi-client TCP Echo Server using `select()`.
- `select_server`: compiled artifact currently kept as submitted evidence.

## Current learning focus

```text
socket() -> server_sock
              |
           select()
              | ready
           accept()
              |
         client_sock
              |
    FD_SET(client_sock)
              |
       select() monitors it
              |
        read() / write()
```

Key points:

- `server_sock` is the listening fd.
- `accept(server_sock, ...)` returns a new connection fd; the bitmap does not allocate fd values.
- `FD_SET()` manually adds an existing fd to `master_set`.
- `read_set = master_set` prepares the working set for each `select()` call.
- `select()` modifies `read_set` so that only ready fds remain.
- `maxfd + 1` is passed as `nfds`.
- A disconnected client is closed and removed with `FD_CLR()`.

## Build

```bash
g++ select_server.cpp -o select_server
./select_server
```

Test clients can connect with:

```bash
nc 127.0.0.1 8080
```

## Status

This file is a guided learning implementation, not yet an independently reproduced L3 result. The next verification is to rewrite it from an empty file and test at least three clients, disconnect cleanup, and accepting a new client afterward.

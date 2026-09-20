# 2026-09-20 poll / epoll learning

## Files

- `poll_server.cpp`: guided learning snapshot of the poll-based multi-client Echo Server discussed today.
- epoll source is **not** recorded as an independent implementation yet; today only covered the model, `epoll_create1()`, `epoll_ctl()`, `epoll_wait()`, and the listen-fd/client-fd dispatch flow.

## poll model learned today

```text
pollfd[]
  |
  +-- fds[0] = server_sock
  +-- fds[1..] = client_sock or -1
  |
poll()
  |
scan revents
  |
  +-- server_sock ready -> accept() -> find fd == -1 -> add client
  |
  +-- client_sock ready -> read()/write()
                         -> close + fd = -1 on disconnect
```

Key points:

- Empty slots use `fd = -1`; poll ignores entries whose fd is negative.
- `events` is configured by the program; `revents` is filled by `poll()`.
- `server_sock` handles new connections; each `accept()` returns a separate connected `client_sock`.
- One listening socket can correspond to many connected client sockets.
- `poll()` only reports readiness. It does not guarantee that a requested number of TCP bytes has arrived.
- The monitored range passed to `poll()` must match the array region the program intends to manage.

## epoll model started today

```text
epoll_create1()
      |
     epfd
      |
epoll_ctl(ADD)
      |
register server_sock / client_sock
      |
epoll_wait()
      |
returns ready events only
      |
  +---+-------------------+
  |                       |
server_sock            client_sock
  |                       |
accept()                read()
  |                       |
new client_sock          write()
  |
epoll_ctl(ADD)
```

Current focus:

- `epfd` is the epoll instance fd, not a client connection.
- `epoll_ctl()` changes the epoll interest list with ADD/MOD/DEL.
- `epoll_wait()` writes ready events into the returned `events[]` array.
- The event loop still distinguishes `server_sock` from connected client sockets before deciding between `accept()` and `read()`.
- LT is the current epoll mode to learn first; nonblocking + ET remains the next stage.

## Status

This is still guided-learning evidence, not an L3 independent implementation. No independent multi-client runtime output was recorded today.

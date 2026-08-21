# SFS — Server From Scratch

A multithreaded HTTP/1.1 server written in C++17, built directly on POSIX sockets.
No Boost.Asio, no libuv, no third-party HTTP library — the socket setup, request
parser, router, and response serializer are all hand-written.

![C++](https://img.shields.io/badge/C%2B%2B-17-blue)
![Build](https://img.shields.io/badge/build-CMake-informational)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)

---

## Overview

SFS implements the HTTP request/response lifecycle from the raw `socket()` call
upward: accepting TCP connections, parsing request bytes off the wire, matching
them against a route table, and writing a correctly-framed HTTP response back —
all without relying on an existing HTTP stack.

It supports JSON/plain-text/HTML responses, path parameters (`/users/:id`), static
file serving with MIME-type detection, and concurrent request handling via a fixed
worker thread pool.

```
$ curl http://localhost:8080/ping
pong

$ curl http://localhost:8080/info
{
  "server": "Server From Scratch",
  "language": "C++17",
  "worker_threads": 8
}
```

## Features

- **Hand-rolled HTTP/1.1 parser** — reads the request line, headers, and body
  directly off the socket, with bounded buffering (8 KB header cap, 1 MB body
  cap) so a malicious or broken client can't exhaust server memory.
- **Thread pool concurrency** — a single acceptor thread hands each connection
  off to a fixed-size pool of worker threads via a mutex + condition-variable
  task queue, rather than spawning a thread per connection.
- **Method-aware router with path parameters** — routes like `/users/:id` are
  matched segment-by-segment and the extracted values are exposed through
  `Request::get_path()`. Unmatched paths fall through to a static file server;
  a matched path with the wrong method returns `405`, not a generic `404`.
- **Static file server with directory-traversal protection** — resolves the
  requested path against the static root using `weakly_canonical()` and
  rejects any path that escapes the root (`403 Forbidden`), with MIME types
  resolved from a small extension table.
- **Header-safe response builder** — `Content-Length` is computed automatically
  from the body and direct attempts to override `Content-Length` or
  `Connection` are blocked, preventing response-smuggling-style bugs.
- **Exception-safe dispatch** — if a route handler throws, the router catches
  it and returns `500` instead of taking the connection down.
- **Per-connection receive timeout** — a 5-second `SO_RCVTIMEO` on each client
  socket so a slow or stalled client can't tie up a worker thread forever.
- **Structured logging** — lightweight `LOG_INFO` / `LOG_WARN` / `LOG_ERROR`
  macros used throughout for connection and request lifecycle visibility.

## Architecture

```
 Client                Server                  ThreadPool              Router
   │      TCP connect     │                         │                     │
   ├──────────────────────▶  accept()                │                     │
   │                       ├────────enqueue()────────▶                     │
   │                       │                         │  worker picks up    │
   │                       │                         ├──handle_client()────▶
   │                       │                         │                     │  dispatch()
   │                       │                         │                     ├──────▶ Handler
   │                       │                         │                     ◀──────┤
   │   HTTP response       │                         │                     │
   ◀───────────────────────┴─────────────────────────┴─────────────────────┘
```

| Component    | Responsibility                                                            |
|--------------|----------------------------------------------------------------------------|
| `Server`     | Socket setup (`bind`/`listen`), accept loop, dispatches to the pool        |
| `ThreadPool` | Fixed worker pool draining a thread-safe task queue                        |
| `Request`    | Parses the raw socket stream into method, path, headers, query, body      |
| `Router`     | Matches method + path (incl. `:param` segments), 404/405 handling         |
| `Response`   | Builds and serializes a well-formed HTTP/1.1 response                     |
| `FileServer` | Serves static assets with MIME detection and traversal protection         |
| `Logger`     | Macro-based structured logging (`LOG_INFO`, `LOG_WARN`, `LOG_ERROR`)       |

**Concurrency model:** one acceptor thread, N worker threads (`N = max(4, hardware_concurrency())`
by default). The acceptor only calls `accept()` and `enqueue()` — all parsing,
routing, and I/O for a connection happens on a worker thread, so a slow client
never blocks new connections from being accepted.

## Project structure

```
SFS/
├── include/
│   ├── file_server.hpp
│   ├── logger.hpp
│   ├── request.hpp
│   ├── response.hpp
│   ├── router.hpp
│   ├── server.hpp
│   └── thread_pool.hpp
├── public/
│   ├── index.html
│   ├── benchmark.html
│   └── style.css
├── src/
│   ├── file_server.cpp
│   ├── main.cpp
│   ├── request.cpp
│   ├── response.cpp
│   ├── router.cpp
│   ├── server.cpp
│   └── thread_pool.cpp
├── utils/
├── build/
├── Dockerfile
├── render.yaml
├── CMakeLists.txt
└── README.md
```

## Getting started

### Prerequisites

- A C++17 compiler (GCC ≥ 9 or Clang ≥ 10)
- CMake ≥ 3.15
- A POSIX environment (Linux or macOS — uses `sys/socket.h`, `unistd.h`, etc.)

### Build

```bash
git clone <your-repo-url> sfs
cd sfs
mkdir build && cd build
cmake ..
make -j$(nproc)
```

> Adjust the binary name below to whatever target your `CMakeLists.txt` defines.

### Run

```bash
./sfs
```

```
[INFO] server started at port 8080 with 8 worker threads
[INFO] server accepting connections at http://localhost:8080
```

Then open **http://localhost:8080** — that's `public/index.html`, served by SFS itself.

## API reference

### Built-in demo routes (`main.cpp`)

| Method | Path     | Description                                              |
|--------|----------|-----------------------------------------------------------|
| GET    | `/ping`  | Liveness check — returns `pong`                            |
| GET    | `/info`  | Returns server metadata as JSON (language, thread count)   |
| GET    | `/echo`  | Echoes back the `msg` query parameter                      |
| GET    | `/tasks` | Returns an in-memory list of tasks as JSON                 |
| POST   | `/tasks` | Adds a new task to the in-memory list (send body as text)  |
| GET    | `/*`     | Falls through to the static file server (`public/`)        |

### Defining your own routes

```cpp
server.get("/users/:id", [](const Request &req) {
    std::string id = req.get_path("id");
    return Response::json("{\"id\": \"" + id + "\"}");
});
```

Path segments prefixed with `:` are captured automatically by the router and
retrieved with `req.get_path("name")`. Query parameters use `req.get_query("name")`,
and headers use `req.get_header("Name")` (case-insensitive).

## Security considerations

- Header and body sizes are capped (8 KB / 1 MB) to bound memory use per request.
- Static file paths are canonicalized and checked against the static root before
  the file is opened, blocking `../../etc/passwd`-style traversal attempts.
- `Content-Length` and `Connection` headers cannot be set manually on a `Response`,
  preventing accidental or malicious response framing bugs.
- Route handlers are wrapped in `try/catch` — an uncaught exception becomes a
  `500`, not a crashed worker thread.

## Known limitations

These are deliberate scope cuts for a from-scratch learning project, not oversights:

- No persistent connections — every request closes the socket after one response
  (no `Connection: keep-alive` / pipelining support yet).
- No TLS — HTTP only, no HTTPS.
- Blocking I/O per worker thread, not an event loop (`epoll`/`io_uring`) — simpler
  to reason about, but won't scale to tens of thousands of concurrent connections.
- No chunked transfer-encoding on incoming requests (only `Content-Length`-based
  bodies are read).

## Benchmarking

`public/benchmark.html` is a load-testing dashboard served by SFS itself —
open `http://localhost:8080/benchmark.html` (or your deployed URL) to use it.

- Fires real `fetch()` requests at a configurable concurrency and total count,
  against any path on the server (defaults to `/ping`).
- Reports throughput (req/s), success/failure counts, and latency percentiles
  (p50 / p95 / p99 / max), plus a per-request scatter plot and a latency
  histogram — all hand-rolled SVG, no charting library.
- Same-origin by default, so there's no CORS configuration needed. You *can*
  point it at a different origin, but that target would need to send
  `Access-Control-Allow-Origin` headers itself.

**Honest caveat:** browsers cap concurrent connections per origin (around 6
for HTTP/1.1), so this won't show you the server's true ceiling at high
concurrency settings — it's a correctness/demo tool, not a substitute for a
real load generator. For an actual ceiling, run from a terminal instead:

```bash
wrk -t4 -c100 -d10s http://localhost:8080/ping
# or
ab -n 5000 -c 100 http://localhost:8080/ping
```

## Deploying

SFS binds directly to a raw POSIX socket, so it deploys cleanly as a plain
Docker container on any platform that can run one and expose a port.

### Build & run locally with Docker

```bash
docker build -t sfs .
docker run -p 8080:8080 sfs
```

> Check the `BINARY_NAME` build arg in the `Dockerfile` — it must match
> whatever executable name your `CMakeLists.txt` target actually produces.

## License

MIT — see [LICENSE](LICENSE).

## Author

Built as a from-scratch systems programming project to understand the HTTP
protocol and concurrent server design at the socket level, with no framework
in between.

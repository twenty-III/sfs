# MyCppServer

A multithreaded HTTP/1.1 server written from scratch in C++17. No external libraries — just POSIX sockets, the standard library, and about 600 lines of code.

Built to understand what actually happens between a browser sending a request and your handler function running. Every layer — TCP socket, byte parsing, routing, thread scheduling — is written by hand and explained in comments.

---

## How it works

```
Client                        Server (one per request, on a worker thread)
  │                                │
  │──── TCP connect ───────────────▶│  accept() returns a new file descriptor
  │                                │
  │──── raw bytes ─────────────────▶│  recv() in a loop until \r\n\r\n
  │  "GET /users/42 HTTP/1.1\r\n   │
  │   Host: localhost\r\n\r\n"     │  RequestParser splits method / path /
  │                                │  headers / body into a Request object
  │                                │
  │                                │  Router walks its route list,
  │                                │  matches "/users/:id", extracts id="42",
  │                                │  calls the registered handler lambda
  │                                │
  │◀─── raw bytes ─────────────────│  Response::serialize() builds the
  │  "HTTP/1.1 200 OK\r\n          │  wire format; send() loops until
  │   Content-Length: 37\r\n\r\n…" │  all bytes are written
  │                                │
  │──── TCP close ─────────────────▶│  close(client_fd)
```

---

## Quick start

**Requirements:** Linux, GCC ≥ 9, CMake ≥ 3.16

```bash
git clone https://github.com/yourname/cpp-projects
cd cpp-projects

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target http_server_demo -j$(nproc)

# Run from the project root so ./public resolves correctly
cd build/http_server && ./http_server_demo
```

Open [http://localhost:8080](http://localhost:8080) — the demo page lets you test every endpoint live from the browser.

---

## Project structure

```
http_server/
├── include/
│   ├── server.hpp        # Server class — socket, thread pool, file mounts
│   ├── request.hpp       # Request (immutable value object) + RequestParser
│   ├── response.hpp      # Response builder + factory methods
│   ├── router.hpp        # Route registration and dispatch
│   ├── thread_pool.hpp   # Fixed worker threads + condition_variable queue
│   ├── file_server.hpp   # Static file serving from disk
│   └── logger.hpp        # Thread-safe logger (Meyers singleton)
├── src/
│   ├── server.cpp
│   ├── request.cpp
│   ├── response.cpp
│   ├── router.cpp
│   ├── thread_pool.cpp
│   ├── file_server.cpp
│   ├── logger.cpp
│   └── main.cpp          # Demo routes
└── public/               # Served as static files
    ├── index.html
    └── style.css
```

---

## API — demo endpoints

| Method | Path | What it demonstrates |
|--------|------|----------------------|
| GET | `/ping` | Health check — returns `pong` |
| GET | `/info` | Live JSON with server version and thread count |
| GET | `/echo?msg=…` | Query parameter parsing |
| GET | `/users/:id` | Path parameter extraction |
| POST | `/users` | Reading the request body |
| DELETE | `/users/:id` | 204 No Content response |

```bash
curl http://localhost:8080/ping

curl http://localhost:8080/info

curl "http://localhost:8080/echo?msg=hello"

curl http://localhost:8080/users/42

curl -X POST http://localhost:8080/users \
     -H "Content-Type: application/json" \
     -d '{"name": "Alice"}'

curl -s -o /dev/null -w "%{http_code}" \
     -X DELETE http://localhost:8080/users/42
```

---

## Adding a route

Register handlers before calling `server.listen()`. Handlers are lambdas that take a `const Request&` and return a `Response`:

```cpp
// Path parameter
server.get("/items/:id", [](const Request& req) {
    std::string id = req.path_params().at("id");
    return Response::json("{\"id\": \"" + id + "\"}");
});

// Query parameter
server.get("/search", [](const Request& req) {
    auto& p = req.query_params();
    std::string q = p.count("q") ? p.at("q") : "";
    return Response::ok("Searching for: " + q);
});

// Request body
server.post("/data", [](const Request& req) {
    Response res = Response::json("{\"got\": " + std::to_string(req.body().size()) + "}");
    res.status_code = 201;
    return res;
});

// Serve a directory of static files
server.serve_files("/", "./public");
```

---

## C++ concepts covered

**Sockets and systems programming**
- `socket()`, `bind()`, `listen()`, `accept()`, `recv()`, `send()` — the full POSIX socket lifecycle
- `SO_REUSEADDR` — why restarting a server immediately fails without it
- `SO_RCVTIMEO` — read timeout to protect against slow-loris connections
- `MSG_NOSIGNAL` — why `send()` on a closed socket kills your process without this flag
- Byte order: `htons()` and why network byte order (big-endian) differs from x86

**Concurrency**
- `std::thread`, `std::mutex`, `std::condition_variable` — built from scratch, no thread-per-request
- Why workers use `condition_variable::wait()` instead of spinning
- Double-checked locking pattern for read-heavy shared state
- `mutable` — how to lock a mutex in a `const` method

**Modern C++**
- `std::string_view` — non-owning string references; when to use instead of `const string&`
- Move semantics — `std::move` in `Response` factories avoids copying large bodies
- C++17 fold expressions `(oss << ... << args)` — the variadic logger in one line
- `if constexpr` — compile-time branch to handle empty parameter packs
- `std::filesystem` — path manipulation and atomic file writes via `rename()`
- Structured bindings `auto& [key, val]` in range-for loops

**Design patterns**
- Meyers singleton — thread-safe without a mutex on the instance itself
- Value objects — `Request` is immutable after construction; handlers get `const Request&`
- Friend classes — `RequestParser` and `Router` write private fields; handlers cannot
- Factory methods — `Response::json()`, `Response::not_found()` guarantee consistent state
- Separation of concerns — `shortener.cpp` has zero HTTP knowledge; `main.cpp` is the glue

---

## Design decisions

**Why `std::string_view` for parameters we don't store, `std::string` by value for parameters we do?**

`const string&` forces a heap allocation even for a string literal. `string_view` is a non-owning pointer + length — zero cost at the call site. When we do need to own the string (storing in a map, setting as `body`), taking by value lets the caller move a temporary in with zero copies.

**Why a static library instead of a shared library?**

Static linking bundles all the code directly into the app binary. There's no `.so` to manage, no runtime linker path to set, and Docker images are simpler. For a collection of demo apps this is the right tradeoff.

**Why hand-roll the JSON instead of using a library?**

The JSON we produce is simple enough that a string-building approach is 10 lines and has no failure modes. Adding a JSON library (nlohmann, RapidJSON) would be the right call for production code with complex nested structures.

**Why first-match-wins routing instead of specificity-based?**

Specificity rules (like Express.js) require scoring each route and sorting. First-match-wins is O(n) and completely predictable — if `/users/me` is registered before `/users/:id`, it always wins. The rule "register more specific routes first" is easy to remember and hard to get wrong.

---

## Using as a library

The server builds as `http_server_lib` (static). Any app in this monorepo can link against it:

```cmake
# In your app's CMakeLists.txt:
target_link_libraries(my_app http_server_lib)
# Automatically gets http_server/include/ on the include path.
```

See `apps/url_shortener/` for a working example — a URL shortener built on top of this server where the shortener itself has no knowledge of HTTP.

---

## License

MIT

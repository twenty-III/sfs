# cpp-projects

A monorepo containing a C++ HTTP server built from scratch, and apps built on top of it.

## Structure

```
cpp_projects/
├── http_server/        # The framework — compiled as a static library
│   ├── include/        # Public headers (request, response, router, server, ...)
│   └── src/            # Implementation
└── apps/
    └── url_shortener/  # App 1 — no HTTP knowledge, just business logic + glue
        ├── src/
        │   ├── shortener.hpp/cpp   # pure C++ data store
        │   └── main.cpp            # wires HTTP routes to the shortener
        └── public/                 # frontend served from disk
```

## Build locally

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Run from the project root so ./public resolves correctly
cd build/apps/url_shortener && ./url_shortener
```

Open http://localhost:8080

## API

| Method | Path | Description |
|--------|------|-------------|
| POST | /api/shorten | Body: plain URL. Returns JSON with short code. |
| GET | /s/:code | 301 redirect to original URL |
| GET | /api/stats | `{"total": N}` |

```bash
# Shorten
curl -X POST http://localhost:8080/api/shorten -d "https://github.com"

# Redirect (follow with -L)
curl -L http://localhost:8080/s/abc123

# Stats
curl http://localhost:8080/api/stats
```

## Deploy with Docker

```bash
# Build
docker build -t url-shortener .

# Run
docker run -p 8080:8080 url-shortener

# Push to Docker Hub
docker tag url-shortener yourname/url-shortener:latest
docker push yourname/url-shortener:latest
```

## Deploy to Railway.app (free tier)

1. Push this repo to GitHub
2. Go to [railway.app](https://railway.app) → New Project → Deploy from GitHub
3. Select this repo
4. Railway auto-detects the Dockerfile and deploys
5. Your live URL: `https://your-app.up.railway.app`

> **Note:** Railway's free tier doesn't have persistent volumes.
> `data/urls.json` resets on each redeploy. For production persistence,
> use a database (PostgreSQL, Redis) or a volume mount.

## Adding a new app

1. `mkdir -p apps/new_app/src`
2. Write `apps/new_app/CMakeLists.txt` (copy from url_shortener)
3. Link against `http_server_lib` — you get all headers automatically
4. Uncomment `add_subdirectory(apps/new_app)` in root `CMakeLists.txt`

## Tech notes

- **No external dependencies** — pure C++17 + POSIX
- **Thread pool** — `std::thread` + `std::condition_variable`, N workers where N = CPU cores
- **Thread-safe shortener** — `std::shared_mutex`: concurrent reads, exclusive writes
- **Atomic file saves** — write to `.tmp`, then `rename()` (atomic on POSIX)
- **Logger** — Meyers singleton, `std::shared_mutex`, variadic template with C++17 fold expressions

# ── Build stage ───────────────────────────────────────────────
FROM ubuntu:22.04 AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN mkdir -p build \
    && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
    && make -j"$(nproc)"

# ── Runtime stage ─────────────────────────────────────────────
FROM ubuntu:22.04

# Only the C++ standard library is needed at runtime (static-ish binary
# from a typical CMake build still dynamically links libstdc++/libgcc).
RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

# ── ⚠️  CHECK THIS LINE ──────────────────────────────────────
# Replace "sfs" with whatever executable name your CMakeLists.txt
# target actually produces (check build/ locally after `make` to confirm).
ARG BINARY_NAME=sfs

WORKDIR /app
COPY --from=build /app/build/${BINARY_NAME} ./bin/sfs
COPY --from=build /app/public ./public

# main.cpp calls set_static_dir("../public") relative to the binary's
# working directory — so we run from /app/bin with public/ as its sibling,
# matching the same layout your local `build/` directory already has.
WORKDIR /app/bin

EXPOSE 8080
ENV PORT=8080

CMD ["./sfs"]

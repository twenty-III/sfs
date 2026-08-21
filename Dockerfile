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

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

ARG BINARY_NAME=sfs

WORKDIR /app
COPY --from=build /app/build/${BINARY_NAME} ./bin/sfs
COPY --from=build /app/public ./public

WORKDIR /app/bin

EXPOSE 8080
ENV PORT=8080

CMD ["./sfs"]

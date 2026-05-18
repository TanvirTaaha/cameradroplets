## syntax=docker/dockerfile:1.7

# ==========================================
# STAGE 1: Build Environment
# ==========================================
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# 1. Install core compiling structures + Python3 (required by AWS SDK tools)
RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt/lists,sharing=locked \
    apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    python3 \
    libssl-dev \
    libcurl4-openssl-dev \
    uuid-dev \
    libzstd-dev \
    liblz4-dev \
    ffmpeg \
    libavformat-dev \
    libavcodec-dev \
    libavutil-dev \
    libswscale-dev \
    && rm -rf /var/lib/apt/lists/*

COPY lib_src/ /third_party/
WORKDIR /third_party

# 2. Header-only libraries: use copied local repos first, clone only if missing
RUN --mount=type=cache,target=/root/.cache/git,sharing=locked \
    ( [ -d "concurrentqueue" ] || git clone --depth 1 --branch v1.0.4 https://github.com/cameron314/concurrentqueue.git ) && \
    ( [ -d "json" ] || git clone --depth 1 --branch v3.12.0 https://github.com/nlohmann/json.git )

# 3. Build librdkafka explicitly compiled with active compression flags
RUN --mount=type=cache,target=/root/.cache/git,sharing=locked \
    ( [ -d "librdkafka" ] || git clone https://github.com/confluentinc/librdkafka.git ) && \
    cd librdkafka && \
    ./configure --enable-zstd --enable-lz4 && \
    make -j$(nproc) && make install

# 4. Clone and build AWS C++ SDK with full source tree (shallow) to keep build arch-neutral
# Sparse checkouts can miss required generated/src trees in newer releases.
RUN --mount=type=cache,target=/root/.cache/git,sharing=locked \
    ( [ -d "aws-sdk-cpp" ] || git clone --depth 1 --recurse-submodules --shallow-submodules https://github.com/aws/aws-sdk-cpp.git ) \
    && cd aws-sdk-cpp && git submodule update --init --recursive --depth 1 && mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DBUILD_ONLY="s3" \
          -DAWS_CUSTOM_MEMORY_MANAGEMENT=0 \
          -DBUILD_SHARED_LIBS=ON .. && \
    make -j$(nproc) && make install

# 5. Compile your application source tree
WORKDIR /app
COPY . .

RUN rm -rf build && mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

# ==========================================
# STAGE 2: Final Optimized Runtime Container
# ==========================================
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt/lists,sharing=locked \
    apt-get update && apt-get install -y \
    libssl3 \
    libcurl4 \
    libzstd1 \
    liblz4-1 \
    ffmpeg \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Pull only the clean dynamic libs and application target
COPY --from=builder /usr/local/lib /usr/local/lib
COPY --from=builder /app/build/camdrops /app/camdrops

RUN ldconfig

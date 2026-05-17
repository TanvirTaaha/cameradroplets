# ==========================================
# STAGE 1: Borrow Prebuilt Optimized FFmpeg Components
# ==========================================
FROM jrottenberg/ffmpeg:7.1-ubuntu2404 AS ffmpeg_source

# ==========================================
# STAGE 2: Build Environment
# ==========================================
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# 1. Install core compiling structures and package properties
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    libssl-dev \
    libcurl4-openssl-dev \
    libboost-all-dev \
    uuid-dev \
    # High-performance compression libraries for Kafka
    libzstd-dev \
    liblz4-dev \
    && rm -rf /var/lib/apt/lists/*

# 2. Copy the optimized FFmpeg headers and shared objects directly from jrottenberg
COPY --from=ffmpeg_source /usr/local/include /usr/local/include
COPY --from=ffmpeg_source /usr/local/lib /usr/local/lib
COPY --from=ffmpeg_source /usr/local/bin/ffmpeg /usr/local/bin/ffmpeg

# 3. Pull Header-Only and source wrappers
WORKDIR /third_party

# Position moodycamel exactly where CMake checks for it
RUN git clone https://github.com/cameron314/concurrentqueue.git && \
    mkdir -p /usr/local/include/moodycamel && \
    cp concurrentqueue/concurrentqueue.h /usr/local/include/moodycamel/ && \
    cp concurrentqueue/blockingconcurrentqueue.h /usr/local/include/moodycamel/

# Build librdkafka explicitly compiled with active compression flags
RUN git clone https://github.com/confluentinc/librdkafka.git && \
    cd librdkafka && \
    ./configure --enable-zstd --enable-lz4 && \
    make -j$(nproc) && make install

# Install nlohmann/json
RUN git clone https://github.com/nlohmann/json.git && \
    cd json && mkdir build && cd build && \
    cmake -DJSON_BuildTests=OFF .. && make install

# Compile AWS C++ SDK (Scoped completely to S3 to save build time)
RUN git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp.git && \
    cd aws-sdk-cpp && mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_ONLY="s3" -DAWS_CUSTOM_MEMORY_MANAGEMENT=0 .. && \
    make -j$(nproc) && make install

# 4. Compile your application source tree
WORKDIR /app
COPY . .

RUN rm -rf build && mkdir build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make -j$(nproc)

# ==========================================
# STAGE 3: Final Optimized Runtime Container
# ==========================================
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Install core system shared dependencies (Fixed package names for Ubuntu 24.04)
RUN apt-get update && apt-get install -y \
    libssl3 \
    libcurl4 \
    libzstd1 \
    liblz4-1 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Pull only the compiled libraries and binaries (keeping the container lightweight)
COPY --from=builder /usr/local/lib /usr/local/lib
COPY --from=builder /app/build/camdrops /app/camdrops

# Register our custom library bindings folder inside the OS dynamic cache
RUN ldconfig

ENTRYPOINT ["./camdrops"]
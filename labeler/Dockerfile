# Build and run the MIP File SDK C++ labeler on Ubuntu 22.04
# Uses native .so libraries from the Ubuntu NuGet package.

FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    libcurl4-openssl-dev \
    libssl-dev \
    libsecret-1-dev \
    uuid-dev \
    libglib2.0-dev \
    libgsf-1-dev \
    libgmime-3.0-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Copy MIP SDK headers and libs from NuGet cache (mounted or copied in)
COPY mip_sdk/ /mip_sdk/

# Copy labeler source
COPY labeler/src/ /build/src/
COPY labeler/CMakeLists.txt /build/

# Build the labeler
RUN cmake -B out \
    -DMIP_SDK_PATH=/mip_sdk \
    -DCMAKE_BUILD_TYPE=Release \
    && cmake --build out --parallel

# --- Runtime stage ---
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    libcurl4 \
    libssl3 \
    libsecret-1-0 \
    uuid-runtime \
    libglib2.0-0 \
    libgsf-1-114 \
    libgmime-3.0-0 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy built binary and MIP SDK shared libraries
COPY --from=builder /build/out/mip-sdk-cli /app/mip-sdk-cli
COPY --from=builder /build/out/*.so* /app/

# Add /app to library search path
ENV LD_LIBRARY_PATH=/app

ENTRYPOINT ["/app/mip-sdk-cli"]

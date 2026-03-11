# ─────────────────────────────────────────────────────────────────────────────
# Stage 1 — Build
# ─────────────────────────────────────────────────────────────────────────────
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        libopencv-dev \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

# Copy the entire backend directory
COPY backend/ .

# Build (Release mode)
RUN cmake -B build -S . -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --parallel "$(nproc)"

# ─────────────────────────────────────────────────────────────────────────────
# Stage 2 — Runtime (lean image)
# ─────────────────────────────────────────────────────────────────────────────
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Only runtime OpenCV libraries needed
RUN apt-get update && apt-get install -y --no-install-recommends \
        libopencv-core4.5d \
        libopencv-imgcodecs4.5d \
        libopencv-imgproc4.5d \
        libopencv-dnn4.5d \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy the compiled binary
COPY --from=builder /src/build/SudokuBackend ./SudokuBackend

# Copy ONNX model if it exists (optional — manual solve works without it)
COPY --from=builder /src/assets ./assets 2>/dev/null || true

# Render / Railway inject PORT; default to 8080 locally
ENV PORT=8080
EXPOSE 8080

CMD ["./SudokuBackend"]

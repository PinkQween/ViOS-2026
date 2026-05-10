FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc-x86-64-linux-gnu \
    nasm \
    xorriso \
    mtools \
    qemu-system-x86 \
    qemu-utils \
    make \
    curl \
    git \
    unzip \
    ca-certificates \
    python3 \
    && rm -rf /var/lib/apt/lists/*

# Build xmake
RUN curl -fsSL https://xmake.io/shget.text | bash

ENV PATH="/root/.local/bin:${PATH}"

WORKDIR /workspace
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    g++ \
    libcpprest-dev \
    libssl-dev \
    libboost-system-dev \
    inotify-tools \
    curl \
    lsof \
    apache2-utils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY src /app

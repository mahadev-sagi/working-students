FROM node:20-alpine AS certs

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Copy CA certificates from Alpine (no network needed)
COPY --from=certs /etc/ssl/certs/ca-certificates.crt /etc/ssl/certs/ca-certificates.crt

# Switch to HTTPS mirrors (port 80 is blocked on your network)
RUN sed -i 's|http://archive.ubuntu.com|https://mirrors.edge.kernel.org|g' /etc/apt/sources.list.d/ubuntu.sources && \
    sed -i 's|http://security.ubuntu.com|https://mirrors.edge.kernel.org|g' /etc/apt/sources.list.d/ubuntu.sources

# Install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    libpq-dev \
    libssl-dev \
    libpistache-dev \
    postgresql-client \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# ---- Install header-only libraries ----

# jwt-cpp (v0.7.0)
RUN git clone --branch v0.7.0 --depth 1 https://github.com/Thalhammer/jwt-cpp.git /tmp/jwt-cpp && \
    cp -r /tmp/jwt-cpp/include/jwt-cpp /usr/local/include/ && \
    rm -rf /tmp/jwt-cpp

# picojson (header-only)
RUN mkdir -p /usr/local/include/picojson && \
    wget https://raw.githubusercontent.com/kazuho/picojson/master/picojson.h \
    -O /usr/local/include/picojson/picojson.h

# ---- Build and install libbcrypt ----
RUN git clone https://github.com/trusch/libbcrypt.git /tmp/libbcrypt && \
    cd /tmp/libbcrypt && \
    mkdir build && cd build && \
    cmake .. && make && make install && \
    rm -rf /tmp/libbcrypt

# ---- Copy project and build ----
COPY . /app

RUN mkdir -p build && cd build && \
    cmake .. && \
    make

EXPOSE 9080

# Add wait-for-db script
COPY wait-for-db.sh /wait-for-db.sh
RUN sed -i 's/\r$//' /wait-for-db.sh
RUN chmod +x /wait-for-db.sh

ENTRYPOINT ["/wait-for-db.sh"]
CMD ["/app/build/server"]
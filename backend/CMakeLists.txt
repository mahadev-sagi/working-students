cmake_minimum_required(VERSION 3.10)
project(workingstudents_backend)

set(CMAKE_CXX_STANDARD 17)

# Source files
set(SOURCES
    src/server.cpp
    src/db.cpp
    src/auth.cpp
)

# Include directories
include_directories(
    /usr/local/include
    /usr/include
    /usr/include/postgresql
    src
)

# Find required packages
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# Executable
add_executable(server ${SOURCES})

# JWT config: use picojson (default)

# Link libraries
target_link_libraries(server
    pthread
    pq
    ssl
    crypto
    bcrypt
    pistache
)
# Simple Makefile for Working Students Backend

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra

# Source files
SRCS = src/server.cpp src/db.cpp src/auth.cpp
OBJS = $(SRCS:.cpp=.o)

# Output
TARGET = build/server

# Try to use pkg-config for dependencies
PISTACHE_CFLAGS := $(shell pkg-config --cflags libpistache 2>/dev/null)
PISTACHE_LIBS := $(shell pkg-config --libs libpistache 2>/dev/null)
NLOHMANN_CFLAGS := $(shell pkg-config --cflags nlohmann_json 2>/dev/null)
NLOHMANN_LIBS := $(shell pkg-config --libs nlohmann_json 2>/dev/null)
POSTGRES_CFLAGS := $(shell pkg-config --cflags libpq 2>/dev/null)
POSTGRES_LIBS := $(shell pkg-config --libs libpq 2>/dev/null)

# Fallbacks for Homebrew (macOS)
ifeq ($(PISTACHE_CFLAGS),)
PISTACHE_CFLAGS = -I/usr/local/opt/pistache/include
PISTACHE_LIBS = -L/usr/local/opt/pistache/lib -lpistache
endif
ifeq ($(NLOHMANN_CFLAGS),)
NLOHMANN_CFLAGS = -I/usr/local/opt/nlohmann-json/include
NLOHMANN_LIBS =
endif
ifeq ($(POSTGRES_CFLAGS),)
POSTGRES_CFLAGS = -I/usr/local/opt/postgresql@18/include/postgresql
POSTGRES_LIBS = -L/usr/local/opt/postgresql@18/lib/postgresql -lpq
endif

# jwt-cpp (header only)
JWT_CPP_CFLAGS = -Ivendor/jwt-cpp/include

# bcrypt and other libs
BCRYPT_LIB = vendor/bcrypt/bcrypt.a
EXTRA_LIBS = -lcrypto -lssl -pthread

# Combine all flags
INCLUDES = $(PISTACHE_CFLAGS) $(NLOHMANN_CFLAGS) $(POSTGRES_CFLAGS) $(JWT_CPP_CFLAGS) -Ivendor/bcrypt
LIBS = $(PISTACHE_LIBS) $(NLOHMANN_LIBS) $(POSTGRES_LIBS) $(BCRYPT_LIB) $(EXTRA_LIBS)

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean

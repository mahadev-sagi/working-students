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
PISTACHE_CFLAGS = -I/opt/homebrew/include -I/opt/homebrew/Cellar/pistache/0.4.26/include
PISTACHE_LIBS = -L/opt/homebrew/lib -lpistache
endif
ifeq ($(NLOHMANN_CFLAGS),)
NLOHMANN_CFLAGS = -I/opt/homebrew/Cellar/nlohmann-json/3.12.0/include
NLOHMANN_LIBS =
endif
ifeq ($(POSTGRES_CFLAGS),)
POSTGRES_CFLAGS = -I/opt/homebrew/Cellar/postgresql@18/18.3/include/postgresql
POSTGRES_LIBS = -L/opt/homebrew/Cellar/postgresql@18/18.3/lib/postgresql -lpq
endif

# bcrypt and other libs
BCRYPT_LIB = /opt/homebrew/lib/bcrypt.a
EXTRA_LIBS = -lcrypto -lssl -pthread

# Combine all flags
INCLUDES = $(PISTACHE_CFLAGS) $(NLOHMANN_CFLAGS) $(POSTGRES_CFLAGS)
LIBS = $(PISTACHE_LIBS) $(NLOHMANN_LIBS) $(POSTGRES_LIBS) $(BCRYPT_LIB) $(EXTRA_LIBS)

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean

#!/bin/sh
set -e

HOST="${DB_HOST:-db}"
PORT="${DB_PORT:-5432}"

echo "Checking database at $HOST:$PORT..."

i=0
MAX_ATTEMPTS=15

while [ $i -lt $MAX_ATTEMPTS ]; do
  if pg_isready -h "$HOST" -p "$PORT" 2>/dev/null; then
    echo "Database is ready!"
    exec "$@"
  fi
  i=$((i + 1))
  echo "Attempt $i/$MAX_ATTEMPTS - waiting..."
  sleep 2
done

echo "Could not confirm database readiness, starting anyway..."
exec "$@"
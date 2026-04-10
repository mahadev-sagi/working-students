#!/bin/sh
# wait-for-db.sh
set -e
host="$1"
shift
until pg_isready -h "$host" -p 5432 -U ws_app; do
  >&2 echo "Postgres is unavailable - sleeping"
  sleep 1
done
>&2 echo "Postgres is up - executing command"
exec "$@"

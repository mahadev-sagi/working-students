# Authentication and account provisioning

Set a stable random `JWT_SECRET` in the backend environment before starting.
The root README documents JWT setup and rotation.

## Public endpoints

- `POST /login`: existing accounts with enabled private passwords only.
- `GET /me`: requires a valid bearer token and an enabled, existing account.
- `POST /signup`: creates new student accounts only. It never activates or changes
  an existing student, even if their password is unset, or an existing admin email.
- `POST /lookup-email` and `POST /set-password`: retired; return 410 without
  looking up an email or modifying a password.

## Initial student passwords

Until expiring, single-use email invitations are implemented, an authenticated
admin verifies the student identity and uses **Roster > Set initial password**.
The password must contain 12 to 72 UTF-8 bytes and be shared privately. The API is
`POST /admin/students/password` with JSON `email` and `password`. It requires an
admin token and an enrollment in a class owned by that admin. One conditional
SQL update prevents overwriting any existing or disabled password and handles
concurrent activation safely. It does not reset existing passwords.

## Published seed credentials and recovery

Seed accounts start locked with password hash `!`. No usable default password is
provided. For an existing database, apply `db-init/disable_published_credentials.sql`
with `psql -v ON_ERROR_STOP=1`; do not rerun the destructive schema initializer.
The migration preserves IDs, enrollments, classes, and ownership. Code also refuses
the previously published seed hash for login and token verification.

The operator-only `account_password` executable updates an existing admin or
student using an email argument and a new password on stdin. It requires explicit
`DB_HOST`, `DB_NAME`, `DB_USER`, and `DB_PASSWORD` environment settings and supports
`DB_PORT` / `DB_SSLMODE`. Supply the password through a secure pipe from a password
manager, not a command argument, terminal echo, or checked-in file. There is no
public admin-password reset or bootstrap endpoint. Rotate JWT_SECRET and redeploy
after recovery to revoke sessions issued under the previous password.

## Verification

Build the backend Docker image, run CTest, then run the HTTP/SQL regression suite:

```sh
docker build -t working-students-auth-fix backend
docker run --rm --network none --entrypoint ctest working-students-auth-fix --test-dir /app/build --output-on-failure
python backend/tests/account_security.py
```

The HTTP suite provisions disposable local Docker containers. It never uses the
root `.env` or connects to the production database.

# Backend setup

## Prerequisites

- Homebrew
- PostgreSQL running with database `productivity_app`
- `pistache`, `libpq`, `nlohmann-json`, `jwt-cpp`, `bcrypt`

Install libs:
```bash
brew install pistache
brew install postgresql
brew install nlohmann-json
```
(install `jwt-cpp` + `bcrypt` via your package manager or submodule)

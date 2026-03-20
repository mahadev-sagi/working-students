

# Authentication API Test Instructions

To test the authentication endpoints with the provided test user, follow these steps:

## Build and start the server

In the backend directory, use the provided Makefile to build the server:

```bash
make
./build/server
```

If you encounter missing dependencies, install them using your system's package manager (see Makefile for details and paths).

# Authentication API Endpoints

## 1. Email Lookup

Check if a user exists and if a password is set:

```bash
curl -X POST http://localhost:9080/lookup-email \
   -H "Content-Type: application/json" \
   -d '{"email":"test@example.com"}'
```

**Response:**
- If email not found: `{ "exists": false }`
- If found and password is set: `{ "exists": true, "hasPassword": true }`
- If found and password is not set: `{ "exists": true, "hasPassword": false }`

## 2. Set Password

Set a password for a user who does not have one:

```bash
curl -X POST http://localhost:9080/set-password \
   -H "Content-Type: application/json" \
   -d '{"email":"test@example.com", "password":"NewPassword123"}'
```

**Response:**
- 200 OK if successful, error if already set or not found.

## 3. Login

Login with email and password (only if password is set):

```bash
curl -X POST http://localhost:9080/login \
   -H "Content-Type: application/json" \
   -d '{"email":"test@example.com", "password":"Test123"}'
```

**Response:**
- JSON with a `token` field if successful, error otherwise.

## 4. Get Current User
Use the token from login to get the current user's info:

```bash
curl -X GET http://localhost:9080/me \
   -H "Authorization: Bearer TOKEN"
```

**Response:**
- JSON with the user's email and id.

## 5. First Sign-On Flow (Test User: new_user@example.com)

All collaborators have access to the ws_app database profile (password: changeme).

You can repeatedly test the first sign-on flow using the special test user new_user@example.com. After each test, reset the password_hash to NULL to allow another first sign-on. (password must be null to test setting new password, so we must clear the password after it is set in each test run)

### a. Lookup Email (first sign-on)

```bash
curl -X POST http://localhost:9080/lookup-email \
   -H "Content-Type: application/json" \
   -d '{"email":"new_user@example.com"}'
```

### b. Set Password (first sign-on)

```bash
curl -X POST http://localhost:9080/set-password \
   -H "Content-Type: application/json" \
   -d '{"email":"new_user@example.com", "password":"YourNewPassword123"}'
```

### c. Login (after setting password)

```bash
curl -X POST http://localhost:9080/login \
   -H "Content-Type: application/json" \
   -d '{"email":"new_user@example.com", "password":"YourNewPassword123"}'
```

### d. Reset password_hash to NULL (for repeated testing)

Run this SQL command as ws_app (password: changeme):

```bash
psql -U ws_app -h localhost -d workingstudents -c "UPDATE students SET password_hash = NULL WHERE email = 'new_user@example.com';"
```

This allows you to repeat the first sign-on flow as many times as needed.

---


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

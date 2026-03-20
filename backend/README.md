
# Authentication API Test Instructions

To test the authentication endpoints with the provided test user, follow these steps:

## 1. Build and start the server

In the backend directory, use the provided Makefile to build the server:

```bash
make
./build/server
```

If you encounter missing dependencies, install them using your system's package manager (see Makefile for details and paths).

## 2. Test /login endpoint

```bash
curl -X POST http://localhost:9080/login \
   -H "Content-Type: application/json" \
   -d '{"email":"test@example.com","password":"Test123"}'
```

This will return a JSON object with a `token` field.

## 3. Test /me endpoint

Copy the token from the previous step and use it in the following command (replace TOKEN with your actual token):

```bash
curl -X GET http://localhost:9080/me \
   -H "Authorization: Bearer TOKEN"
```

You should receive a JSON response with the test user's email and id.

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

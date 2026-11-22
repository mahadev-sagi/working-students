# working-students
Platform targeted to enhance productivity for university students who are also employed outside of school. 

## Run with Docker (Windows and macOS)

The project is designed to run in Docker containers, so teammates on Windows and macOS can use the same workflow.

### Prerequisites

- Docker Desktop installed
- Docker Desktop running
- Linux containers enabled

Windows note: Docker Desktop uses a Linux backend through WSL2. If Docker is installed but not running, compose commands fail with a Docker pipe error.

### Start the stack

From the repository root:

Set `JWT_SECRET` in the untracked root `.env` before starting. Generate a unique
secret with `openssl rand -hex 32` (64 hex characters representing 32 random
bytes). Never commit the value or share it with the frontend. Compose passes
`.env` to the backend. A direct `./build/server` launch requires exporting the
variable in the process environment; the executable does not read `.env` itself.
The backend refuses to start if the secret is missing, shorter than 32 bytes,
whitespace only, or the old compromised placeholder.

```bash
docker compose up -d --build
```

### Rotate the JWT signing secret

Treat the previously hard-coded signing key as compromised. Replace `JWT_SECRET`
with a newly generated value in each backend deployment's environment or secrets
manager, then redeploy/recreate **all** backend instances with the fixed code.
For local Compose, update `.env`, then run:

```bash
docker compose up -d --build --force-recreate backend
```

The key is loaded once at startup. A plain container restart does not pick up
changes to Compose's `.env`. Rotation immediately invalidates all existing
tokens, including forged admin tokens; everyone must log in again. Do not retain
the compromised key as a verification fallback. Confirm an old token returns
401 and a fresh login token succeeds before considering deployment complete.

Earlier login debug output included plaintext passwords and password hashes.
Restrict and purge affected logs according to your retention policy, and reset
credentials exposed in those logs. The updated login path no longer logs request
bodies, credentials, hashes, or tokens.

Services:

Seed accounts have no usable default password. Public email lookup and password
activation are disabled. See [backend account provisioning](backend/README.md)
for private admin recovery and initial student passwords.

- Frontend: http://localhost:3000
- Backend API: http://localhost:9080
- Postgres: localhost:5432

### Stop the stack

```bash
docker compose down
```

To also remove database volume data:

```bash
docker compose down -v
```

### Demo seed for Neon

If you want a presentation-ready dataset in Neon, first apply the schema from `db-init/workingstudents.sql`, then load the demo data:

```bash
psql "$DATABASE_URL" -f db-init/demo_seed.sql
```

The demo seed resets the app tables and inserts a sample student, classes, shifts, and upcoming assignments that work well with the current calendar and assignments UI.

### Troubleshooting on Windows

If you see an error like:

`open //./pipe/dockerDesktopLinuxEngine: The system cannot find the file specified`

do this:

1. Open Docker Desktop and wait until it shows it is running.
2. Confirm WSL2 is available:
	- `wsl --status`
3. Re-run:
	- `docker info`
	- `docker compose up -d --build`

### C++ IntelliSense setup for mixed OS teams

VS Code C/C++ settings now include separate configurations for Win32, Mac, and Linux in [.vscode/c_cpp_properties.json](.vscode/c_cpp_properties.json). This avoids the Windows error caused by a Mac-only compiler path while keeping Mac compatibility.

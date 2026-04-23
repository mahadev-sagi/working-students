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

```bash
docker compose up -d --build
```

Services:

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

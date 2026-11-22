"""HTTP/SQL regression tests in disposable Docker containers, never a live DB.

Run after docker build -t working-students-auth-fix backend:
    python backend/tests/account_security.py
"""
import json
import subprocess
import time
import urllib.error
import urllib.request
import uuid


def docker(*args, data=None, check=True):
    return subprocess.run(["docker", *args], input=data, text=True,
                          capture_output=True, check=check).stdout.strip()


def main():
    suffix = uuid.uuid4().hex[:10]
    network, db, app = (f"auth-{name}-{suffix}" for name in ("net", "db", "app"))
    image = "working-students-auth-fix"
    try:
        docker("network", "create", network)
        docker("run", "-d", "--name", db, "--network", network,
               "-e", "POSTGRES_PASSWORD=test-only-db-password", "postgres:16")
        for _ in range(60):
            ready = subprocess.run(["docker", "exec", db, "pg_isready", "-U", "postgres"],
                                   capture_output=True).returncode == 0
            if ready:
                break
            time.sleep(0.5)
        else:
            raise RuntimeError("Fixture database did not become ready")

        def sql(statement):
            return docker("exec", "-i", db, "psql", "-X", "-U", "postgres", "-At",
                          "-v", "ON_ERROR_STOP=1", data=statement)

        sql("""
            CREATE TABLE students(id SERIAL PRIMARY KEY, name TEXT, email TEXT UNIQUE, password_hash TEXT);
            CREATE TABLE admin_users(id SERIAL PRIMARY KEY, name TEXT, email TEXT UNIQUE, password_hash TEXT NOT NULL);
            CREATE TABLE classes(id SERIAL PRIMARY KEY, instructor_id INTEGER REFERENCES admin_users(id));
            CREATE TABLE enrollments(student_id INTEGER REFERENCES students(id), class_id INTEGER REFERENCES classes(id));
            INSERT INTO admin_users(email,password_hash) VALUES ('owner@example.test','!'),('other@example.test','!');
            INSERT INTO students(email) VALUES ('pending@example.test'),('unrelated@example.test');
            INSERT INTO classes(instructor_id) VALUES (1);
            INSERT INTO enrollments VALUES (1,1);
        """)
        env = ["-e", f"DB_HOST={db}", "-e", "DB_PORT=5432", "-e", "DB_NAME=postgres",
               "-e", "DB_USER=postgres", "-e", "DB_PASSWORD=test-only-db-password",
               "-e", "DB_SSLMODE=disable", "-e", "JWT_SECRET=test-only-signing-secret-0123456789abcdef"]
        for email in ("owner@example.test", "other@example.test"):
            docker("run", "--rm", "-i", "--network", network, *env,
                   "--entrypoint", "/app/build/account_password", image, "admin", email,
                   data="test-only-admin-password\n")
        docker("run", "-d", "--name", app, "--network", network, *env,
               "-p", "127.0.0.1::9080", image)
        address = docker("port", app, "9080/tcp").splitlines()[0]
        base = f"http://{address}"

        def request(path, payload=None, token=None):
            headers = {"Content-Type": "application/json"}
            if token:
                headers["Authorization"] = "Bearer " + token
            body = json.dumps(payload).encode() if payload is not None else None
            req = urllib.request.Request(base + path, data=body, headers=headers)
            try:
                with urllib.request.urlopen(req, timeout=5) as response:
                    return response.status, response.read().decode()
            except urllib.error.HTTPError as error:
                return error.code, error.read().decode()

        for _ in range(60):
            try:
                if request("/me")[0] == 401:
                    break
            except (OSError, TimeoutError):
                pass
            time.sleep(0.5)
        else:
            raise RuntimeError("Fixture backend did not become ready")

        for path in ("/lookup-email", "/set-password"):
            first = request(path, {"email": "pending@example.test", "password": "attacker-password"})
            assert first[0] == 410
            assert request(path, {"email": "absent@example.test"}) == first
        assert request("/signup", {"email": "pending@example.test", "password": "attacker-password"})[0] == 409
        assert request("/signup", {"email": "owner@example.test", "password": "attacker-password"})[0] == 409
        assert sql("SELECT password_hash IS NULL FROM students WHERE email='pending@example.test';") == "t"
        assert request("/signup", {"email": "new@example.test", "password": "test-only-student-password"})[0] == 200

        def login(email, password):
            status, body = request("/login", {"email": email, "password": password})
            assert status == 200, "Fixture login failed"
            return json.loads(body)["token"]

        owner = login("owner@example.test", "test-only-admin-password")
        other = login("other@example.test", "test-only-admin-password")
        student = login("new@example.test", "test-only-student-password")
        payload = {"email": "pending@example.test", "password": "test-only-initial-password"}
        assert request("/admin/students/password", payload)[0] == 401
        assert request("/admin/students/password", payload, student)[0] == 403
        assert request("/admin/students/password", payload, other)[0] == 403
        assert request("/admin/students/password", {**payload, "email": "unrelated@example.test"}, owner)[0] == 403
        assert request("/admin/students/password", {**payload, "password": "short"}, owner)[0] == 400
        assert request("/admin/students/password", payload, owner)[0] == 200
        assert request("/admin/students/password", {**payload, "password": "overwrite-attempt"}, owner)[0] == 403
        pending = login("pending@example.test", "test-only-initial-password")
        assert request("/me", token=pending)[0] == 200
        sql("UPDATE admin_users SET password_hash='!' WHERE id=1;")
        assert request("/me", token=owner)[0] == 401
        sql("DELETE FROM admin_users WHERE id=2;")
        assert request("/me", token=other)[0] == 401
        print("Account security checks passed: public activation, signup takeover, scoped admin provisioning, and revocation.")
    finally:
        docker("rm", "-f", app, check=False)
        docker("rm", "-f", db, check=False)
        docker("network", "rm", network, check=False)


if __name__ == "__main__":
    main()

CREATE DATABASE productivity_app;
CREATE TABLE events (
    id SERIAL PRIMARY KEY,
    title VARCHAR(255) NOT NULL,
    category VARCHAR(100),
    event_date DATE,
    start_time TIME,
    end_time TIME,
    location VARCHAR(255)
);

CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    name TEXT,
    email TEXT UNIQUE,
    password_hash TEXT,
    role TEXT DEFAULT 'student'
);

CREATE TABLE assignment_types (
    id SERIAL PRIMARY KEY,
    type_name TEXT,
    avg_completion_hours INTEGER
);
INSERT INTO assignment_types (type_name, avg_completion_hours)
VALUES
('homework', 2),
('quiz', 1),
('essay', 5),
('research paper', 8),
('lab report', 4),
('discussion post', 1);
CREATE TABLE assignments (
    id SERIAL PRIMARY KEY,
    user_id INTEGER REFERENCES users(id),
    title TEXT,
    due_date DATE,
    predicted_hours INTEGER
);
SELECT * FROM users;
SELECT current_database();
SELECT datname FROM pg_database;

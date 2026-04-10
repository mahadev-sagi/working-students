--CREATE DATABASE workingstudents;

DROP TABLE IF EXISTS events CASCADE;
DROP TABLE IF EXISTS student_assignments CASCADE;
DROP TABLE IF EXISTS assignments CASCADE;
DROP TABLE IF EXISTS assignment_types CASCADE;
DROP TABLE IF EXISTS enrollments CASCADE;
DROP TABLE IF EXISTS classes CASCADE;
DROP TABLE IF EXISTS students CASCADE;

-- if integrity of database is compromised, this will drop all tables and start fresh


-- STUDENTS

CREATE TABLE students (
    id SERIAL PRIMARY KEY,
    name TEXT,
    email TEXT UNIQUE,
    password_hash TEXT DEFAULT NULL -- can be NULL for students created by admin without setting password
);

INSERT INTO students (name, email, password_hash)
VALUES
('test', 'test@ufl.edu', '$2b$12$ZJQCE.hYb2JxDIfd2sS2lORLYwelcUME00Ab77clhB/Gx5S5Z8yTi'),  -- password is 'Test123' hashed with bcrypt
('new_user', 'new_user@ufl.edu', NULL); -- example of student created by admin without setting password, can be updated later when student sets password


-- ADMIN USERS

CREATE TABLE admin_users (
    id SERIAL PRIMARY KEY,
    name TEXT,
    email TEXT UNIQUE,
    password_hash TEXT NOT NULL
);
-- CLASSES

CREATE TABLE classes (
    id SERIAL PRIMARY KEY,
    class_name TEXT NOT NULL,
    course_code TEXT,
    instructor_id INTEGER REFERENCES admin_users(id) ON DELETE SET NULL, -- if instructor deleted, set to NULL
    building TEXT,
    room_number TEXT,
    start_time TIME,
    end_time TIME,
    days_of_week TEXT -- need to decide format (e.g., 'MWF', 'TTh', 'Mon/Wed/Fri')
);


-- SHIFTS

CREATE TABLE shifts (
    id SERIAL PRIMARY KEY,
    student_id INTEGER REFERENCES students(id) ON DELETE CASCADE, -- if student deleted, deleted here as well
    start_time TIME,
    end_time TIME,
    day_of_week TEXT, -- need to decide format (e.g., 'M', 'Mon.', 'Monday')
    location TEXT,
    recurring BOOLEAN DEFAULT FALSE -- if true, shift occurs every week on the specified day
);

-- ENROLLMENTS (Many-to-Many)

CREATE TABLE enrollments (
    student_id INTEGER REFERENCES students(id) ON DELETE CASCADE, --if student deleted, deleted here as well
    class_id INTEGER REFERENCES classes(id) ON DELETE CASCADE, --if class deleted, deleted here as well
    PRIMARY KEY (student_id, class_id) --maps students to classes, many-to-many relationship
);


-- ASSIGNMENT TYPES (Time Prediction)

CREATE TABLE assignment_types (
    id SERIAL PRIMARY KEY,
    type_name TEXT UNIQUE NOT NULL,
    avg_completion_hours INTEGER NOT NULL
);

INSERT INTO assignment_types (type_name, avg_completion_hours)
VALUES
('homework', 2),
('quiz', 1),
('essay', 5),
('research paper', 8),
('lab report', 4),
('discussion post', 1);


-- CLASS ASSIGNMENTS

CREATE TABLE assignments (
    id SERIAL PRIMARY KEY,
    class_id INTEGER REFERENCES classes(id) ON DELETE CASCADE, -- can be used to find all assignments for given class
    assignment_type_id INTEGER REFERENCES assignment_types(id),
    assignment_time_avg INTEGER,
    assignment_time_prediction INTEGER DEFAULT 0, 
    -- default to 0, if still 0 reference avg time prediction for that assignment type, 
    -- perhaps admin account can provide a more accurate prediction at the time they 
    -- enter a new assignment into the system, which will update this to a value other than 0
    title TEXT,
    description TEXT DEFAULT '',
    due_time TIME,
    due_date DATE
);


SELECT * FROM students;
SELECT current_database();
SELECT datname FROM pg_database;

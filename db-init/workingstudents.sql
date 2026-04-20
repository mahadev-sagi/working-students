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

INSERT INTO admin_users (name, email, password_hash)
VALUES
('Admin Test', 'admin_test@ufl.edu', '$2b$12$ZJQCE.hYb2JxDIfd2sS2lORLYwelcUME00Ab77clhB/Gx5S5Z8yTi'),
('Course Admin', 'course_admin@ufl.edu', '$2b$12$ZJQCE.hYb2JxDIfd2sS2lORLYwelcUME00Ab77clhB/Gx5S5Z8yTi');
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

-- STUDENT ASSIGNMENT COMPLETIONS (Historical Tracking)

CREATE TABLE student_assignments (
    id SERIAL PRIMARY KEY,
    student_id INTEGER REFERENCES students(id) ON DELETE CASCADE,
    assignment_id INTEGER REFERENCES assignments(id) ON DELETE CASCADE,
    actual_completion_hours NUMERIC(6,2) NOT NULL,
    completed_at TIMESTAMP DEFAULT NOW(),
    UNIQUE(student_id, assignment_id)
);


-- Travel Prediction Algorithm: Campus Locations and Paths

-- CAMPUS LOCATIONS (Nodes in the graph)
CREATE TABLE campus_locations (
    id SERIAL PRIMARY KEY,
    location_code TEXT UNIQUE NOT NULL,
    location_name TEXT NOT NULL,
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_location_code ON campus_locations(location_code);

-- CAMPUS PATHS (Undirected edges in the graph)
CREATE TABLE campus_paths (
    id SERIAL PRIMARY KEY,
    from_location_id INTEGER NOT NULL REFERENCES campus_locations(id) ON DELETE CASCADE,
    to_location_id INTEGER NOT NULL REFERENCES campus_locations(id) ON DELETE CASCADE,
    distance_meters INTEGER NOT NULL,
    through_indoors BOOLEAN DEFAULT FALSE,
    estimated_obstacles INTEGER DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(from_location_id, to_location_id),
    CHECK (from_location_id < to_location_id)
);

CREATE INDEX idx_path_from ON campus_paths(from_location_id);
CREATE INDEX idx_path_to ON campus_paths(to_location_id);

-- TRAVEL TIME CACHE
CREATE TABLE travel_time_cache (
    id SERIAL PRIMARY KEY,
    from_location_id INTEGER NOT NULL REFERENCES campus_locations(id) ON DELETE CASCADE,
    to_location_id INTEGER NOT NULL REFERENCES campus_locations(id) ON DELETE CASCADE,
    distance_meters INTEGER NOT NULL,
    travel_time_minutes INTEGER NOT NULL,
    query_count INTEGER DEFAULT 1,
    cached_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP + INTERVAL '7 days'
);

CREATE INDEX idx_cache_route ON travel_time_cache(from_location_id, to_location_id);
CREATE INDEX idx_cache_expires ON travel_time_cache(expires_at);

-- Add foreign key to classes table
ALTER TABLE classes ADD COLUMN location_id INTEGER REFERENCES campus_locations(id);

-- Add foreign key to shifts table  
ALTER TABLE shifts ADD COLUMN location_id INTEGER REFERENCES campus_locations(id);

INSERT INTO campus_locations (id, location_code, location_name) VALUES
    (1, 'CSE', 'Computer Science/Engineering Building'),
    (2, 'NPB', 'Physics Building'),
    (3, 'HUB', 'The Hub'),
    (4, 'CLB', 'Chemistry Lab Building'),
    (5, 'TUR', 'Turlington Hall'),
    (6, 'MSL', 'Marston Library'),
    (7, 'LBW', 'Library West'),
    (8, 'REI', 'Reitz Student Union'),
    (9, 'MALA', 'Malachowsky Hall'),
    (10, 'MCC', 'McCarty Hall Buildings'),
    (11, 'NRN', 'Norman Hall'),
    (12, 'LIT', 'Little Hall'),
    (13, 'CAR', 'Carleton Auditorium'),
    (14, 'FA', 'Fine Arts Buildings'),
    (15, 'PUGH', 'Pugh Hall'),
    (16, 'WEIL', 'Weil Hall');

-- Reset sequence to match explicit IDs
SELECT setval('campus_locations_id_seq', (SELECT MAX(id) FROM campus_locations));

-- Insert all campus paths for the travel graph
INSERT INTO campus_paths (from_location_id, to_location_id, distance_meters) VALUES
    (1, 2, 800),
    (1, 3, 75),
    (1, 4, 320),
    (1, 5, 220),
    (1, 6, 200),
    (1, 7, 480),
    (1, 8, 480),
    (1, 9, 640),
    (1, 10, 240),
    (1, 11, 800),
    (1, 12, 480),
    (1, 13, 320),
    (1, 14, 480),
    (1, 15, 210),
    (1, 16, 320),
    (2, 3, 800),
    (2, 4, 1130),
    (2, 5, 970),
    (2, 6, 970),
    (2, 7, 1450),
    (2, 8, 320),
    (2, 9, 480),
    (2, 10, 640),
    (2, 11, 1610),
    (2, 12, 1290),
    (2, 13, 1130),
    (2, 14, 1130),
    (2, 15, 800),
    (2, 16, 640),
    (3, 4, 320),
    (3, 5, 210),
    (3, 6, 240),
    (3, 7, 480),
    (3, 8, 480),
    (3, 9, 480),
    (3, 10, 320),
    (3, 11, 970),
    (3, 12, 480),
    (3, 13, 320),
    (3, 14, 640),
    (3, 15, 150),
    (3, 16, 210),
    (4, 5, 320),
    (4, 6, 480),
    (4, 7, 320),
    (4, 8, 800),
    (4, 9, 970),
    (4, 10, 480),
    (4, 11, 1130),
    (4, 12, 640),
    (4, 13, 480),
    (4, 14, 800),
    (4, 15, 320),
    (4, 16, 640),
    (5, 6, 110),
    (5, 7, 320),
    (5, 8, 800),
    (5, 9, 800),
    (5, 10, 480),
    (5, 11, 800),
    (5, 12, 320),
    (5, 13, 240),
    (5, 14, 480),
    (5, 15, 320),
    (5, 16, 480),
    (6, 7, 480),
    (6, 8, 640),
    (6, 9, 640),
    (6, 10, 320),
    (6, 11, 640),
    (6, 12, 320),
    (6, 13, 240),
    (6, 14, 320),
    (6, 15, 320),
    (6, 16, 480),
    (7, 8, 1130),
    (7, 9, 1130),
    (7, 10, 800),
    (7, 11, 970),
    (7, 12, 480),
    (7, 13, 320),
    (7, 14, 640),
    (7, 15, 480),
    (7, 16, 800),
    (8, 9, 480),
    (8, 10, 480),
    (8, 11, 1290),
    (8, 12, 970),
    (8, 13, 970),
    (8, 14, 970),
    (8, 15, 640),
    (8, 16, 480),
    (9, 10, 480),
    (9, 11, 1290),
    (9, 12, 970),
    (9, 13, 970),
    (9, 14, 800),
    (9, 15, 640),
    (9, 16, 640),
    (10, 11, 800),
    (10, 12, 640),
    (10, 13, 640),
    (10, 14, 480),
    (10, 15, 480),
    (10, 16, 480),
    (11, 12, 480),
    (11, 13, 640),
    (11, 14, 320),
    (11, 15, 1130),
    (11, 16, 1290),
    (12, 13, 120),
    (12, 14, 240),
    (12, 15, 640),
    (12, 16, 640),
    (13, 14, 270),
    (13, 15, 640),
    (13, 16, 640),
    (14, 15, 800),
    (14, 16, 800),
    (15, 16, 240);

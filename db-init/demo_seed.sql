-- Demo seed script for Neon/Postgres
-- Run after the schema has been created.
-- This resets the app data and inserts a presentation-friendly dataset.

BEGIN;

-- Users
--INSERT INTO admin_users (name, email, password_hash)
--VALUES
 -- ('Neha Rani', 'neharani@ufl.edu', '$2b$12$ZJQCE.hYb2JxDIfd2sS2lORLYwelcUME00Ab77clhB/Gx5S5Z8yTi');

INSERT INTO students (name, email, password_hash)
VALUES
    ('alberta', 'alberta@ufl.edu', NULL); -- password is NULL to demonstrate admin-created student without password, can be set later when student sets password


-- Classes for the demo student
INSERT INTO classes (class_name, course_code, instructor_id, building, room_number, start_time, end_time, days_of_week)
VALUES
  ('Software Engineering', 'CEN 3031', 2, 'CSE', '101', '10:30', '11:45', 'MW'),
  ('Calculus I', 'MAC 2311', 2, 'WEIL', '200', '14:30', '15:20', 'MWF'),
  ('Digital Logic', 'EEL 3701', 2, 'CSE', '220', '16:05', '17:20', 'MW');

-- Enroll demo student in all demo classes
INSERT INTO enrollments (student_id, class_id)
SELECT s.id, c.id
FROM students s
CROSS JOIN classes c
WHERE s.email = 'alberta@ufl.edu';

-- Student work shifts
INSERT INTO shifts (student_id, start_time, end_time, day_of_week, location, recurring)
VALUES
  ((SELECT id FROM students WHERE email = 'alberta@ufl.edu'), '16:00', '19:00', 'Tue', 'Library West', TRUE),
  ((SELECT id FROM students WHERE email = 'alberta@ufl.edu'), '08:00', '12:00', 'Thu', 'Reitz Union', TRUE);

-- Upcoming assignments, spaced so the widget looks realistic in a presentation
INSERT INTO assignments (class_id, assignment_type_id, assignment_time_prediction, title, description, due_time, due_date)
VALUES
  ((SELECT id FROM classes WHERE course_code = 'CEN 3031' LIMIT 1), (SELECT id FROM assignment_types WHERE type_name = 'essay' LIMIT 1), 5, 'Project Proposal', 'Outline project scope and milestones.', '23:59', CURRENT_DATE + INTERVAL '2 days'),
  ((SELECT id FROM classes WHERE course_code = 'CEN 3031' LIMIT 1), (SELECT id FROM assignment_types WHERE type_name = 'homework' LIMIT 1), 2, 'Sprint Backlog', 'Break down implementation tasks.', '23:59', CURRENT_DATE + INTERVAL '5 days'),
  ((SELECT id FROM classes WHERE course_code = 'COP 3503' LIMIT 1), (SELECT id FROM assignment_types WHERE type_name = 'lab report' LIMIT 1), 4, 'Lab 7 Report', 'Submit lab observations and answers.', '23:59', CURRENT_DATE + INTERVAL '1 day'),
  ((SELECT id FROM classes WHERE course_code = 'COP 3503' LIMIT 1), (SELECT id FROM assignment_types WHERE type_name = 'quiz' LIMIT 1), 1, 'Quiz 4', 'Timed quiz on recursion.', '09:00', CURRENT_DATE + INTERVAL '4 days'),
  ((SELECT id FROM classes WHERE course_code = 'ENC 1101' LIMIT 1), (SELECT id FROM assignment_types WHERE type_name = 'discussion post' LIMIT 1), 1, 'Peer Response', 'Respond to two classmates.', '23:59', CURRENT_DATE + INTERVAL '3 days'),
  ((SELECT id FROM classes WHERE course_code = 'ENC 1101' LIMIT 1), (SELECT id FROM assignment_types WHERE type_name = 'essay' LIMIT 1), 6, 'Reflection Draft', 'Draft a reflective essay.', '23:59', CURRENT_DATE + INTERVAL '7 days'),
  ((SELECT id FROM classes WHERE course_code = 'MAC 2311' LIMIT 1), (SELECT id FROM assignment_types WHERE type_name = 'homework' LIMIT 1), 2, 'Integration Set', 'Practice integration problems.', '23:59', CURRENT_DATE + INTERVAL '2 days'),
  ((SELECT id FROM classes WHERE course_code = 'MAC 2311' LIMIT 1), (SELECT id FROM assignment_types WHERE type_name = 'homework' LIMIT 1), 3, 'Series Practice', 'Series and convergence drill.', '23:59', CURRENT_DATE + INTERVAL '6 days'),
  ((SELECT id FROM classes WHERE course_code = 'EEL 3701' LIMIT 1), (SELECT id FROM assignment_types WHERE type_name = 'homework' LIMIT 1), 3, 'Logic Problem Set', 'Boolean algebra and gates.', '23:59', CURRENT_DATE + INTERVAL '4 days'),
  ((SELECT id FROM classes WHERE course_code = 'COM 3150' LIMIT 1), (SELECT id FROM assignment_types WHERE type_name = 'discussion post' LIMIT 1), 1, 'Industry Post', 'Discuss a workplace communication scenario.', '23:59', CURRENT_DATE + INTERVAL '8 days');

COMMIT;

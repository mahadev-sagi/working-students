-- Non-destructive containment for existing databases. Preserve IDs and ownership.
-- Run with psql -v ON_ERROR_STOP=1. Restore chosen accounts with account_password.
-- Rotate JWT_SECRET and redeploy after applying to invalidate all existing sessions.
BEGIN;
UPDATE admin_users SET password_hash = '!'
WHERE password_hash = '$2b$12$ZJQCE.hYb2JxDIfd2sS2lORLYwelcUME00Ab77clhB/Gx5S5Z8yTi';
UPDATE students SET password_hash = '!'
WHERE password_hash = '$2b$12$ZJQCE.hYb2JxDIfd2sS2lORLYwelcUME00Ab77clhB/Gx5S5Z8yTi';
COMMIT;

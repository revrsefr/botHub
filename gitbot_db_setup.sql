-- Create a PostgreSQL user with password
CREATE USER gitbot_user WITH PASSWORD 'huevo';

-- Create the database owned by gitbot_user
CREATE DATABASE gitbot OWNER gitbot_user;

-- Optionally grant all privileges on the new database
GRANT ALL PRIVILEGES ON DATABASE gitbot TO gitbot_user;

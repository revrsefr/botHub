#!/bin/bash
# ...existing code...
# Run psql commands as postgres user to avoid "role reverse does not exist" error.
sudo -u postgres psql -c "CREATE USER gitbot_user WITH PASSWORD 'huevo';"
sudo -u postgres psql -c "CREATE DATABASE gitbot OWNER gitbot_user;"
sudo -u postgres psql -c "GRANT ALL PRIVILEGES ON DATABASE gitbot TO gitbot_user;"

# Create tables 'private' and 'publictables' in the gitbot database
sudo -u postgres psql -d gitbot -c "CREATE TABLE private (id SERIAL PRIMARY KEY, content TEXT);"
sudo -u postgres psql -d gitbot -c "CREATE TABLE publictables (id SERIAL PRIMARY KEY, content TEXT);"

# Grant all privileges on the new tables to gitbot_user
sudo -u postgres psql -d gitbot -c "GRANT ALL PRIVILEGES ON TABLE private TO gitbot_user;"
sudo -u postgres psql -d gitbot -c "GRANT ALL PRIVILEGES ON TABLE publictables TO gitbot_user;"

echo "Database and tables setup complete."
# ...existing code...
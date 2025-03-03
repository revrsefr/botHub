#include "common.h"
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>

void initialize_database() {
    try {
        spdlog::info("🔍 Attempting to connect to database with: {}", DB_CONN);

        pqxx::connection conn(DB_CONN);
        if (conn.is_open()) {
            spdlog::info("✅ Connected to PostgreSQL database: {}", conn.dbname());
        } else {
            spdlog::error("❌ Failed to connect to database!");
            return;
        }

        pqxx::work txn(conn);
        txn.exec(R"(
            CREATE TABLE IF NOT EXISTS admins (
                id SERIAL PRIMARY KEY,
                hostmask TEXT UNIQUE NOT NULL
            );
            CREATE TABLE IF NOT EXISTS commits (
                id SERIAL PRIMARY KEY,
                repo_name TEXT NOT NULL,
                sha TEXT NOT NULL,
                author TEXT NOT NULL,
                message TEXT NOT NULL,
                url TEXT NOT NULL,
                additions INT DEFAULT 0,
                deletions INT DEFAULT 0,
                changes INT DEFAULT 0
            );
        )");
        txn.commit();
        spdlog::info("✅ Database initialized successfully.");
    } catch (const std::exception& e) {
        spdlog::error("❌ Database initialization error: {}", e.what());
    }
}

void store_commit_info(const std::string& repo, const std::string& sha, const std::string& author, const std::string& message, const std::string& url, int additions, int deletions, int changes) {
    try {
        pqxx::connection conn(DB_CONN);
        if (conn.is_open()) {
            spdlog::info("Connected to PostgreSQL for inserting commit.");
        } else {
            spdlog::error("Failed to connect to database!");
            return;
        }

        pqxx::work txn(conn);
        spdlog::info("Storing commit: {} - {}", repo, sha);

        txn.exec("INSERT INTO commits (repo_name, sha, author, message, url, additions, deletions, changes) VALUES (" +
                 txn.quote(repo) + ", " + txn.quote(sha) + ", " + txn.quote(author) + ", " +
                 txn.quote(message) + ", " + txn.quote(url) + ", " +
                 txn.quote(additions) + ", " + txn.quote(deletions) + ", " + txn.quote(changes) + ");");

        txn.commit();
        spdlog::info("Commit stored successfully.");
    } catch (const std::exception& e) {
        spdlog::error("Database error: {}", e.what());
    }
}

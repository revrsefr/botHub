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

void store_commit_info(const std::string& repo, const std::string& sha, const std::string& author, 
                       const std::string& message, const std::string& url, int additions, 
                       int deletions, int changes) {
    try {
        pqxx::connection conn(DB_CONN);
        pqxx::work txn(conn);

        txn.exec_params(
            "INSERT INTO commits (repo_name, sha, author, commit_hash, message, timestamp) "
            "VALUES ($1, $2, $3, $4, $5, CURRENT_TIMESTAMP) "
            "ON CONFLICT (commit_hash) DO NOTHING;",  // Prevent duplicate commits
            repo, sha, author, sha, message
        );

        txn.commit();
        spdlog::info("✅ Commit stored: [{}] {} - {}", repo, sha, message);
    } catch (const std::exception& e) {
        spdlog::error("❌ Database error while storing commit: {}", e.what());
    }
}

bool is_commit_stored(const std::string& repo, const std::string& sha) {
    try {
        pqxx::connection conn(DB_CONN);
        pqxx::work txn(conn);
        pqxx::result res = txn.exec(
            "SELECT 1 FROM commits WHERE repo_name = " + txn.quote(repo) + " AND sha = " + txn.quote(sha) + " LIMIT 1;"
        );
        return !res.empty();
    } catch (const std::exception& e) {
        spdlog::error("❌ Database error while checking commit: {}", e.what());
        return false;
    }
}

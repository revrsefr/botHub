
#include "common.h"
#include "config.h"
#include "irc_api.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>
#include <QObject>
#include <QTimer>

using json = nlohmann::json;


// ✅ Get the list of tracked repositories from the database
std::vector<std::string> get_tracked_repos() {
    std::vector<std::string> repos;
    try {
        pqxx::connection conn(DB_CONN);
        pqxx::work txn(conn);

        pqxx::result res = txn.exec("SELECT repo_name FROM tracked_repos;");
        for (const auto& row : res) {
            std::string repo = row[0].as<std::string>();
            repos.push_back(repo);
            
            // 🛑 Debugging log
            spdlog::info("🔍 Tracked repo from DB: {}", repo);
        }
    } catch (const std::exception& e) {
        spdlog::error("❌ Error fetching tracked repositories: {}", e.what());
    }
    return repos;
}

void start_commit_checker() {
    spdlog::info("✅ Starting commit checker every 2 minutes...");
    QTimer* timer = new QTimer();
    QObject::connect(timer, &QTimer::timeout, []() {
        check_for_new_commits();
    });
    timer->start(120000);  // ✅ Check every 2 minutes
}

// ✅ Fetch the latest commit from GitHub API and send it to IRC
void check_for_new_commits() {
    std::vector<std::string> repos = get_tracked_repos();

    for (const std::string& repo : repos) {
        try {
            pqxx::connection conn(DB_CONN);
            pqxx::work txn(conn);

            // ✅ Get last processed commit from the database
            pqxx::result res = txn.exec_params("SELECT last_commit_sha FROM tracked_repos WHERE repo_name = $1;", repo);
            std::string last_commit_sha = res.empty() ? "" : res[0][0].as<std::string>();

            // ✅ Fetch the last 3 commits from GitHub
            std::string url = "https://api.github.com/repos/" + repo + "/commits?per_page=3";
            cpr::Header headers = {{"User-Agent", "C++-GitHub-Bot"}};
            if (!GITHUB_API_KEY.empty()) {
                headers["Authorization"] = "token " + GITHUB_API_KEY;
            }

            auto response = cpr::Get(cpr::Url{url}, headers);

            if (response.status_code == 200) {
                json commits = json::parse(response.text);
                bool found_new_commit = false;
                std::vector<std::string> new_commits;

                for (const auto& commit : commits) {
                    std::string sha = commit["sha"].get<std::string>();
                    std::string author = commit["commit"]["author"]["name"].get<std::string>();
                    std::string message = commit["commit"]["message"].get<std::string>();
                    std::string commit_url = "https://github.com/" + repo + "/commit/" + sha;

                    if (sha == last_commit_sha) {
                        break;  // ✅ Stop if we reach the last known commit
                    }

                    found_new_commit = true;

                    // ✅ Store commit in database
                    store_commit_info(repo, sha, author, message, commit_url, 0, 0, 0);

                    // ✅ Store commit message for sending later
                    std::string irc_message = "[" + repo + "] " + author + " " + sha.substr(0, 7) +
                                              " - " + message + " (" + commit_url + ")";
                    new_commits.push_back(irc_message);
                }

                // ✅ Send messages in **chronological order** (oldest → newest)
                std::reverse(new_commits.begin(), new_commits.end());
                for (const auto& msg : new_commits) {
                    send_irc_message(msg);
                }

                // ✅ Update last known commit **only if new commits were found**
                if (found_new_commit && !commits.empty()) {
                    std::string new_commit_sha = commits[0]["sha"].get<std::string>();
                    txn.exec_params("UPDATE tracked_repos SET last_commit_sha = $1 WHERE repo_name = $2;", new_commit_sha, repo);
                    txn.commit();
                    spdlog::info("✅ Updated last commit for {} to {}", repo, new_commit_sha);
                }

            } else {
                spdlog::error("❌ Failed to fetch commits for {}. HTTP Status: {}", repo, response.status_code);
            }

        } catch (const std::exception& e) {
            spdlog::error("❌ Error processing commits for {}: {}", repo, e.what());
        }
    }
}

// ✅ Fetch the latest commit **live from GitHub API**
std::string get_last_commit(const std::string& repo) {
    std::string url = "https://api.github.com/repos/" + repo + "/commits?page=1&per_page=1";

    cpr::Header headers = {{"User-Agent", "C++-GitHub-Bot"}};
    if (!GITHUB_API_KEY.empty()) {
        headers["Authorization"] = "token " + GITHUB_API_KEY;
    }

    auto response = cpr::Get(cpr::Url{url}, headers);

    if (response.status_code == 200) {
        try {
            json commits = json::parse(response.text);

            if (!commits.empty()) {
                std::string sha = commits[0]["sha"];
                std::string author = commits[0]["commit"]["author"]["name"];
                std::string message = commits[0]["commit"]["message"];
                std::string commit_url = "https://github.com/" + repo + "/commit/" + sha;

                return "📝 Latest commit: " + sha.substr(0, 7) + " by " + author +
                       " - " + message + " (" + commit_url + ")";
            } else {
                return "⚠️ No commits found for " + repo;
            }
        } catch (const std::exception& e) {
            spdlog::error("Error parsing GitHub API response: {}", e.what());
            return "❌ Error retrieving last commit.";
        }
    } else {
        spdlog::error("Failed to fetch commits. HTTP Status: {}", response.status_code);
        return "❌ Failed to fetch commit from GitHub.";
    }
}

#include "common.h"
#include "config.h"
#include "irc_api.h" 
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>

using json = nlohmann::json;


// ✅ Function to get the list of tracked repositories from the database
std::vector<std::string> get_tracked_repos() {
    std::vector<std::string> repos;
    try {
        pqxx::connection conn(DB_CONN);
        pqxx::work txn(conn);

        pqxx::result res = txn.exec("SELECT DISTINCT repo_name FROM commits;");
        for (const auto& row : res) {
            repos.push_back(row[0].as<std::string>());
        }
    } catch (const std::exception& e) {
        spdlog::error("❌ Error fetching tracked repositories: {}", e.what());
    }
    return repos;
}

// ✅ Function to fetch the latest commit for a repository
void check_for_new_commits(IRCClient* bot, const std::string& repo) {
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

                std::string irc_message = "[" + repo + "] " + author + " " + sha.substr(0, 7) + " - " + message;
                spdlog::info("🔔 New commit found: {}", irc_message);

                // ✅ Use `bot->sendIrcMessage()`
                bot->sendIrcMessage(irc_message);
            }
        } catch (const std::exception& e) {
            spdlog::error("Error parsing GitHub API response: {}", e.what());
        }
    } else {
        spdlog::error("Failed to fetch commits. HTTP Status: {}", response.status_code);
    }
}

// ✅ Check for new commits periodically (every 2 minutes)
void start_commit_checker(IRCClient* bot) {
    static QTimer* timer = new QTimer();
    QObject::connect(timer, &QTimer::timeout, [bot]() {
        std::vector<std::string> repos = get_tracked_repos();
        for (const auto& repo : repos) {
            check_for_new_commits(bot, repo);
        }
    });

    timer->start(120000); // 2 minutes
    spdlog::info("✅ GitHub commit checker started (every 2 min)");
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

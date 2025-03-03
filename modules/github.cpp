
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

        pqxx::result res = txn.exec("SELECT DISTINCT repo_name FROM commits;");
        for (const auto& row : res) {
            repos.push_back(row[0].as<std::string>());
        }
    } catch (const std::exception& e) {
        spdlog::error("❌ Error fetching tracked repositories: {}", e.what());
    }
    return repos;
}
// ✅ Start commit checking every 2 minutes
void start_commit_checker() {
    QTimer* timer = new QTimer();
    QObject::connect(timer, &QTimer::timeout, []() {
        check_for_new_commits(); // ✅ Now declared before use
    });
    timer->start(120000);  // ✅ Check every 2 minutes
}
// ✅ Fetch the latest commit from GitHub API and send it to IRC
void check_for_new_commits() {
    std::vector<std::string> repos = get_tracked_repos();

    for (const std::string& repo : repos) {
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

                    // ✅ Check if commit is already stored
                    if (!is_commit_stored(repo, sha)) {
                        // ✅ Store the commit
                        store_commit_info(repo, sha, author, message, commit_url, 0, 0, 0);

                        // ✅ Format commit message
                        std::string irc_message = "[" + repo + "] " + author + " " + sha.substr(0, 7) +
                                                  " - " + message + " (" + commit_url + ")";

                        // ✅ Send commit notification to the IRC channel
                        send_irc_message(irc_message);
                    }
                }
            } catch (const std::exception& e) {
                spdlog::error("Error parsing GitHub API response: {}", e.what());
            }
        } else {
            spdlog::error("Failed to fetch commits for {}. HTTP Status: {}", repo, response.status_code);
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

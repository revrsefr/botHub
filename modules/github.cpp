#include "common.h"
#include "config.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

void fetch_latest_commit(const std::string& repo) {
    std::string url = "https://api.github.com/repos/" + repo + "/commits?page=1&per_page=1";

    cpr::Header headers = {
        {"User-Agent", "C++-GitHub-Bot"}
    };

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

                store_commit_info(repo, sha, author, message, commit_url, 0, 0, 0);
            }
        } catch (const std::exception& e) {
            spdlog::error("Error parsing GitHub API response: {}", e.what());
        }
    } else {
        spdlog::error("Failed to fetch commits. HTTP Status: {}", response.status_code);
    }
}

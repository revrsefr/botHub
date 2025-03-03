#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>
#include <map>
#include <QObject>
#include <QTimer>
#include "logger.h"
#include "helpers.h"

// === Database Connection ===
extern std::string DB_CONN;

// === IRC Colors ===
extern std::map<std::string, std::string> IRC_COLORS;

// === Functions for Admin System ===
bool is_admin(const std::string& hostmask);
std::string add_admin(const std::string& sender_hostmask, const std::string& new_admin_hostmask);
std::string remove_admin(const std::string& sender_hostmask, const std::string& target_hostmask);

// === GitHub Tracking Functions ===
std::string add_repo(const std::string& sender_hostmask, const std::string& repo);
std::string remove_repo(const std::string& sender_hostmask, const std::string& repo);
std::string get_last_commit(const std::string& repo);

// === Functions for GitHub Events ===
void fetch_latest_commit(const std::string& repo);
std::vector<std::string> get_tracked_repos();

// ✅ Declare function to start commit checking
void start_commit_checker();
void check_for_new_commits();

// ✅ Declare function to send messages to IRC
void send_irc_message(const std::string& message);

// === Database Functions ===
void initialize_database();
void store_commit_info(const std::string& repo, const std::string& sha, const std::string& author, const std::string& message, const std::string& url, int additions, int deletions, int changes);
bool is_commit_stored(const std::string& repo, const std::string& sha);

#endif

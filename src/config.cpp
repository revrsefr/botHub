#include "config.h"
#include <iostream>
#include <fstream>
#include <pugixml.hpp>
#include <spdlog/spdlog.h>
#include <pqxx/pqxx>

// Define global variables
std::string SERVER;
int PORT = 6667;  // Default value
bool SSL_ENABLED = false;
std::string BOT_NICK;
std::string IDENT;
std::string GECOS;
std::string SASL_ACCOUNT;
std::string SASL_PASSWORD;
std::string CHANNELS;
std::string GITHUB_API_KEY;
std::string DB_CONN;
std::map<std::string, std::string> IRC_COLORS;
std::map<std::string, std::string> COMMIT_COLORS;  // ✅ Added commit colors map

// ✅ Load settings from XML config file
void load_config() {
    std::string config_path = "/home/reverse/irc/bots/botHub/conf/config.conf";

    // ✅ Ensure doc is declared inside function scope
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(config_path.c_str());

    if (!result) {
        spdlog::error("❌ Failed to open or parse config file: {}", config_path);
        exit(1);
    }

    // ✅ Read database config
    auto db_node = doc.child("database").child("db");
    std::string dbname = db_node.attribute("name").as_string();
    std::string dbuser = db_node.attribute("user").as_string();
    std::string dbpass = db_node.attribute("password").as_string();
    std::string dbhost = db_node.attribute("host").as_string();

    if (dbname.empty() || dbuser.empty() || dbpass.empty() || dbhost.empty()) {
        spdlog::error("❌ Database config is incomplete! Check config.conf");
        exit(1);
    }

    DB_CONN = "dbname=" + dbname + " user=" + dbuser + " password=" + dbpass + " host=" + dbhost;
    spdlog::info("✅ Loaded database connection string: {}", DB_CONN);

    // ✅ Read IRC settings
    auto irc_node = doc.child("irc").child("server");
    SERVER = irc_node.attribute("name").as_string();
    PORT = irc_node.attribute("port").as_int(6697);
    SSL_ENABLED = irc_node.attribute("ssl").as_bool(false);
    BOT_NICK = irc_node.attribute("bot_nick").as_string();
    SASL_ACCOUNT = irc_node.attribute("sasl_account").as_string();
    SASL_PASSWORD = irc_node.attribute("sasl_password").as_string();
    CHANNELS = irc_node.attribute("channels").as_string();
    IDENT = BOT_NICK;
    GECOS = "GitHub Bot";  // Default GECOS

    spdlog::info("✅ IRC Config Loaded - Server: {}, Port: {}, Channels: {}", SERVER, PORT, CHANNELS);

    // ✅ Load GitHub API key
    pugi::xml_node github = doc.child("github").child("api_key");
    GITHUB_API_KEY = github.attribute("value").as_string();

    if (GITHUB_API_KEY.empty()) {
        spdlog::warn("⚠️ GitHub API key not found in config!");
    } else {
        spdlog::info("✅ GitHub API key loaded.");
    }

    // ✅ Load commit colors from config
    auto colors_node = doc.child("colors");
    for (pugi::xml_node color = colors_node.child("color"); color; color = color.next_sibling("color")) {
        std::string name = color.attribute("name").as_string();
        std::string value = color.attribute("value").as_string();
        IRC_COLORS[name] = value;
    }

    // ✅ Load commit-specific colors
    auto commit_colors_node = doc.child("commit_colors");
    for (pugi::xml_node color = commit_colors_node.child("color"); color; color = color.next_sibling("color")) {
        std::string name = color.attribute("name").as_string();
        std::string color_key = color.attribute("value").as_string();

        if (IRC_COLORS.find(color_key) != IRC_COLORS.end()) {
            COMMIT_COLORS[name] = IRC_COLORS[color_key];
        } else {
            COMMIT_COLORS[name] = IRC_COLORS["reset"]; // Default if not found
        }
    }

    spdlog::info("✅ Colors Loaded - {} colors found", IRC_COLORS.size());
    spdlog::info("✅ Commit Colors Loaded - {} mappings found", COMMIT_COLORS.size());

    // ✅ Load default admin
    auto admin_node = doc.child("admin").child("administrator");
    std::string default_admin = admin_node.attribute("user").as_string();

    if (!default_admin.empty()) {
        spdlog::info("🔧 Default admin from config: {}", default_admin);
        try {
            pqxx::connection conn(DB_CONN);
            pqxx::work txn(conn);
            txn.exec("INSERT INTO admins (hostmask) VALUES (" + txn.quote(default_admin) + ") ON CONFLICT DO NOTHING;");
            txn.commit();
            spdlog::info("✅ Default admin added to database: {}", default_admin);
        } catch (const std::exception& e) {
            spdlog::error("❌ Database error adding default admin: {}", e.what());
        }
    } else {
        spdlog::warn("⚠️ No default admin set in config.");
    }

    spdlog::info("[INFO] Configuration loaded successfully!");
}

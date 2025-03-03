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

// ✅ Load settings from XML config file
void load_config() {
    std::string config_path = "/home/debian/irc/Bots/gitbot++/conf/config.conf";
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(config_path.c_str());

    if (!result) {
        std::cerr << "[ERROR] Failed to open or parse config file: " << config_path << std::endl;
        exit(1);
    }

    auto db_node = doc.child("database").child("db");
    std::string dbname = db_node.attribute("name").as_string();
    std::string dbuser = db_node.attribute("user").as_string();
    std::string dbpass = db_node.attribute("password").as_string();
    std::string dbhost = db_node.attribute("host").as_string();

    // 🔍 DEBUG: Print before assigning
    spdlog::info("DB Name: {}, DB User: {}, DB Host: {}", dbname, dbuser, dbhost);

    // Ensure no missing values
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
    GECOS = "GitHub Bot"; // Default GECOS

    // 🔍 DEBUG OUTPUT
    std::cout << "[DEBUG] SERVER: " << SERVER << std::endl;
    std::cout << "[DEBUG] PORT: " << PORT << std::endl;
    std::cout << "[DEBUG] SSL_ENABLED: " << SSL_ENABLED << std::endl;
    std::cout << "[DEBUG] BOT_NICK: " << BOT_NICK << std::endl;
    std::cout << "[DEBUG] SASL_ACCOUNT: " << SASL_ACCOUNT << std::endl;
    std::cout << "[DEBUG] CHANNELS: " << CHANNELS << std::endl;
    std::cout << "[INFO] Configuration loaded successfully!" << std::endl;

    // ✅ Load GitHub API key
    pugi::xml_node github = doc.child("github").child("api_key");
    GITHUB_API_KEY = github.attribute("value").as_string();
    
    if (GITHUB_API_KEY.empty()) {
        spdlog::warn("⚠️ GitHub API key not found in config!");
    } else {
        spdlog::info("✅ GitHub API key loaded.");
    }
    
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


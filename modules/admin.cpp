#include "config.h"
#include "common.h"
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>

bool is_admin(const std::string& hostmask) {
    try {
        pqxx::connection conn(DB_CONN);
        pqxx::work txn(conn);
        pqxx::result res = txn.exec("SELECT 1 FROM admins WHERE hostmask = " + txn.quote(hostmask) + " LIMIT 1;");

        bool isAdmin = !res.empty();
        spdlog::info("🔍 Admin check for {}: {}", hostmask, isAdmin ? "YES" : "NO");

        return isAdmin;
    } catch (const std::exception& e) {
        spdlog::error("❌ Database error while checking admin: {}", e.what());
        return false;
    }
}

std::string add_admin(const std::string& sender_hostmask, const std::string& new_admin_hostmask) {
    if (!is_admin(sender_hostmask)) {
        return IRC_COLORS["color_red"] + "⚠️ You are not authorized to add admins." + IRC_COLORS["color_reset"];
    }

    try {
        pqxx::connection conn(DB_CONN);
        pqxx::work txn(conn);

        // Check if admin already exists
        pqxx::result res = txn.exec("SELECT 1 FROM admins WHERE hostmask = " + txn.quote(new_admin_hostmask) + " LIMIT 1;");

        if (!res.empty()) {
            return IRC_COLORS["color_yellow"] + "⚠️ " + new_admin_hostmask + " is already an admin." + IRC_COLORS["color_reset"];
        }

        // Insert new admin if not found
        txn.exec("INSERT INTO admins (hostmask) VALUES (" + txn.quote(new_admin_hostmask) + ");");
        txn.commit();

        return IRC_COLORS["color_green"] + "✅ Admin added: " + new_admin_hostmask + IRC_COLORS["color_reset"];
    } catch (const std::exception& e) {
        spdlog::error("❌ Error adding admin: {}", e.what());
        return IRC_COLORS["color_red"] + "⚠️ Failed to add admin." + IRC_COLORS["color_reset"];
    }
}

std::string remove_admin(const std::string& sender_hostmask, const std::string& target_hostmask) {
    if (!is_admin(sender_hostmask)) {
        return IRC_COLORS["color_red"] + "⚠️ You are not authorized to remove admins." + IRC_COLORS["color_reset"];
    }

    try {
        pqxx::connection conn(DB_CONN);
        pqxx::work txn(conn);
        txn.exec("DELETE FROM admins WHERE hostmask = " + txn.quote(target_hostmask) + ";");
        txn.commit();

        return IRC_COLORS["color_red"] + "❌ Admin removed: " + target_hostmask + IRC_COLORS["color_reset"];
    } catch (const std::exception& e) {
        spdlog::error("❌ Error removing admin: {}", e.what());
        return IRC_COLORS["color_red"] + "⚠️ Failed to remove admin." + IRC_COLORS["color_reset"];
    }
}

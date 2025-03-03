#include "irc_api.h"
#include "config.h"
#include "common.h"
#include <spdlog/spdlog.h>
#include <QCoreApplication>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    spdlog::info("Bot started");

    // ✅ Load configuration
    load_config();

    // Initialize Database
    initialize_database();

    // ✅ Initialize and start IRC bot
    IRCClient bot;
    bot.run();

    return app.exec();  // Keeps the Qt event loop running
}

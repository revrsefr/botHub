
#include "irc_api.h"
#include "config.h"
#include "common.h"
#include <spdlog/spdlog.h>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <csignal>
#include <unistd.h>
#include <filesystem>

#define PID_FILE "run/gitbot.pid"

IRCClient* botInstance = nullptr;

// ✅ Write PID to file
void write_pid() {
    std::filesystem::create_directory("run"); // Ensure the directory exists
    QFile file(PID_FILE);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream out(&file);
        out << getpid();
        file.close();
        spdlog::info("✅ PID file written: {}", PID_FILE);
    } else {
        spdlog::error("❌ Failed to write PID file!");
    }
}

// ✅ Read PID from file
pid_t read_pid() {
    QFile file(PID_FILE);
    if (!file.open(QIODevice::ReadOnly)) {
        spdlog::error("❌ Failed to read PID file!");
        return -1;
    }
    QTextStream in(&file);
    pid_t pid = in.readLine().toInt();
    file.close();
    return pid;
}

// ✅ Handle signals (Rehash, Restart, Stop)
void signal_handler(int signum) {
    switch (signum) {
        case SIGUSR1:  // Rehash
            spdlog::info("🔄 Rehashing configuration...");
            load_config();
            break;
        case SIGHUP:  // Restart
            spdlog::info("🔄 Restarting bot...");
            execl("/proc/self/exe", "gitbot", "start", nullptr);
            break;
        case SIGTERM:  // Stop
            spdlog::info("🛑 Stopping bot...");
            QCoreApplication::quit();
            break;
    }
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    
    if (argc < 2) {
        spdlog::error("❌ Usage: ./gitbot <start|rehash|restart|stop>");
        return 1;
    }

    std::string command = argv[1];

    if (command == "start") {
        spdlog::info("Bot starting...");
    
        // Fork process to run in the background
        pid_t pid = fork();
    
        if (pid < 0) {
            spdlog::error("❌ Failed to fork process!");
            return 1;
        }
    
        if (pid > 0) {
            // Parent process: Exit, leaving the bot running in the background
            spdlog::info("✅ Bot started in background with PID {}", pid);
            return 0;
        }
    
        // Child process: Continue running the bot
        setsid();  // Create a new session to detach from terminal
    
        write_pid();
    
        // Redirect standard input/output to /dev/null
        freopen("/dev/null", "r", stdin);
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);
    
        // Set up signal handling
        signal(SIGUSR1, signal_handler);  // Rehash
        signal(SIGHUP, signal_handler);   // Restart
        signal(SIGTERM, signal_handler);  // Stop
    
        // ✅ Load configuration & Database
        load_config();
        initialize_database();
    
        // ✅ Start IRC bot
        IRCClient bot;
        botInstance = &bot;
        bot.run();
    
        return app.exec();  // Keeps the bot running
    }    
    else if (command == "rehash") {
        pid_t pid = read_pid();
        if (pid > 0) {
            kill(pid, SIGUSR1);
            spdlog::info("🔄 Sent rehash signal to PID {}", pid);
        } else {
            spdlog::error("❌ Failed to rehash: No running bot instance found.");
        }
        return 0;
    }
    else if (command == "restart") {
        pid_t pid = read_pid();
        if (pid > 0) {
            kill(pid, SIGHUP);
            spdlog::info("🔄 Sent restart signal to PID {}", pid);
        } else {
            spdlog::error("❌ Failed to restart: No running bot instance found.");
        }
        return 0;
    }
    else if (command == "stop") {
        pid_t pid = read_pid();
        if (pid > 0) {
            kill(pid, SIGTERM);
            spdlog::info("🛑 Sent stop signal to PID {}", pid);
        } else {
            spdlog::error("❌ Failed to stop: No running bot instance found.");
        }
        return 0;
    }
    else {
        spdlog::error("❌ Unknown command: {}", command);
        return 1;
    }
}

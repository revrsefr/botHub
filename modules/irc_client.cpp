#include "irc_api.h"
#include "config.h"
#include "common.h"
#include <spdlog/spdlog.h>
#include <QCoreApplication>
#include <QTimer>
#include <QByteArray>

// ✅ Constructor: Initialize `IrcConnection`
IRCClient::IRCClient(QObject* parent) : QObject(parent) {
    connection = new IrcConnection(this);

    connect(connection, &IrcConnection::connected, this, &IRCClient::onConnected);
    connect(connection, &IrcConnection::disconnected, this, &IRCClient::onDisconnected);
    connect(connection, &IrcConnection::privateMessageReceived, this, &IRCClient::onPrivateMessageReceived);
}

// ✅ Destructor
IRCClient::~IRCClient() {
    delete connection;
}

// ✅ Connect to IRC (SASL Authentication in Registration Phase)
void IRCClient::connectToServer() {
    spdlog::info("Connecting to IRC server: {} on port {}", SERVER, PORT);

    connection->setHost(QString::fromStdString(SERVER));
    connection->setPort(PORT);
    connection->setUserName(QString::fromStdString(IDENT));
    connection->setNickName(QString::fromStdString(BOT_NICK));
    connection->setRealName(QString::fromStdString(GECOS));

    if (SSL_ENABLED) {
        connection->setSecure(true);
        spdlog::info("SSL Enabled");
    }

    connection->open();
}

// ✅ Handle successful connection (SASL Authentication & Channel Join)
void IRCClient::onConnected() {
    spdlog::info("Connected to IRC! Starting SASL authentication...");

    // ✅ Request SASL capability BEFORE sending NICK/USER
    connection->sendRaw("CAP REQ :sasl");

    // ✅ Send NICK and USER, but do NOT send CAP END yet!
    connection->sendCommand(IrcCommand::createNick(QString::fromStdString(BOT_NICK)));
    connection->sendCommand(IrcCommand::createQuote("USER " + QString::fromStdString(IDENT) + " 0 * :" + QString::fromStdString(GECOS)));

    // ✅ Wait for ACK from server
    QTimer::singleShot(2000, this, &IRCClient::startSASLAuth);
}

// ✅ Start SASL Authentication
void IRCClient::startSASLAuth() {
    spdlog::info("SASL capability acknowledged. Sending AUTHENTICATE PLAIN...");
    connection->sendRaw("AUTHENTICATE PLAIN");

    // ✅ Wait for AUTHENTICATE + before sending credentials
    QTimer::singleShot(2000, this, &IRCClient::sendSASLCredentials);
}

// ✅ Send SASL Credentials
void IRCClient::sendSASLCredentials() {
    spdlog::info("SASL server responded with '+'. Sending credentials...");

    QByteArray authString;
    authString.append(SASL_ACCOUNT.c_str());
    authString.append('\0');
    authString.append(SASL_ACCOUNT.c_str());
    authString.append('\0');
    authString.append(SASL_PASSWORD.c_str());

    QByteArray encodedAuth = authString.toBase64();
    connection->sendRaw("AUTHENTICATE " + QString::fromUtf8(encodedAuth));

    // ✅ Wait for SASL success
    QTimer::singleShot(2000, this, &IRCClient::finalizeAuth);
}

// ✅ Finalize Authentication
void IRCClient::finalizeAuth() {
    spdlog::info("Finalizing SASL authentication...");

    // ✅ Send CAP END to complete registration
    connection->sendRaw("CAP END");

    // ✅ Join channels
    QTimer::singleShot(2000, this, &IRCClient::joinChannels);
}

// ✅ Join Channels after authentication
void IRCClient::joinChannels() {
    QStringList channelList = QString::fromStdString(CHANNELS).split(",", Qt::SkipEmptyParts);
    for (const QString& channel : channelList) {
        spdlog::info("Joining channel: {}", channel.toStdString());
        connection->sendCommand(IrcCommand::createJoin(channel));
    }
}

// ✅ Handle disconnection
void IRCClient::onDisconnected() {
    spdlog::error("Disconnected from IRC server.");
    emit disconnected();
}

// ✅ Start bot
void IRCClient::run() {
    connectToServer();
    QCoreApplication::exec();
}

// ✅ Handle Private Messages (Admin Commands)
void IRCClient::onPrivateMessageReceived(IrcPrivateMessage* message) {
    QString nick = message->nick();
    QString host = message->host();
    QString target_channel = message->target();
    QString content = message->content();

    std::string sender_hostmask = nick.toStdString() + "!" + host.toStdString();
    spdlog::info("📩 Private message from {}: {}", sender_hostmask, content.toStdString());

    if (content.startsWith("!admin add ")) {
        std::string new_admin_hostmask = content.mid(11).toStdString();
        std::string response = add_admin(sender_hostmask, new_admin_hostmask);
        connection->sendCommand(IrcCommand::createMessage(target_channel, QString::fromStdString(response)));
    }
    else if (content.startsWith("!admin del ")) {
        std::string target_hostmask = content.mid(11).toStdString();
        std::string response = remove_admin(sender_hostmask, target_hostmask);
        connection->sendCommand(IrcCommand::createMessage(target_channel, QString::fromStdString(response)));
    }
    else if (content.startsWith("!admin check ")) {
        std::string target_hostmask = content.mid(13).toStdString();
        bool admin = is_admin(target_hostmask);
        std::string response = admin
            ? IRC_COLORS["color_green"] + "✅ " + target_hostmask + " is an admin." + IRC_COLORS["color_reset"]
            : IRC_COLORS["color_red"] + "❌ " + target_hostmask + " is NOT an admin." + IRC_COLORS["color_reset"];
        connection->sendCommand(IrcCommand::createMessage(target_channel, QString::fromStdString(response)));
    }
    else if (content.startsWith("!git add ")) {
        if (!is_admin(sender_hostmask)) {
            std::string response = IRC_COLORS["color_red"] + "⚠️ You are not authorized to add repositories." + IRC_COLORS["color_reset"];
            connection->sendCommand(IrcCommand::createMessage(target_channel, QString::fromStdString(response)));
            return;
        }
        std::string repo = content.mid(9).toStdString();
        std::string response = add_repo(sender_hostmask, repo);
        connection->sendCommand(IrcCommand::createMessage(target_channel, QString::fromStdString(response)));
    }
    else if (content.startsWith("!git del ")) {
        if (!is_admin(sender_hostmask)) {
            std::string response = IRC_COLORS["color_red"] + "⚠️ You are not authorized to remove repositories." + IRC_COLORS["color_reset"];
            connection->sendCommand(IrcCommand::createMessage(nick, QString::fromStdString(response)));
            return;
        }
        std::string repo = content.mid(9).toStdString();
        std::string response = remove_repo(sender_hostmask, repo);
        connection->sendCommand(IrcCommand::createMessage(target_channel, QString::fromStdString(response)));
    }
    else if (content.startsWith("!git check last ")) {
        std::string repo = content.mid(16).toStdString();
        std::string response = get_last_commit(repo);  // ✅ Fetch directly from GitHub API
        connection->sendCommand(IrcCommand::createMessage(target_channel, QString::fromStdString(response)));
    }
}

// ✅ Rehash Configuration (No Reconnect)
void IRCClient::rehash() {
    spdlog::info("🔄 Reloading configuration...");
    load_config();
    spdlog::info("✅ Configuration reloaded.");
}

// ✅ Restart Bot (Reconnect)
void IRCClient::restart() {
    spdlog::info("🔄 Restarting bot...");
    connection->quit();
    QTimer::singleShot(3000, this, &IRCClient::connectToServer);
}

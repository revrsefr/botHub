#include "irc_api.h"
#include "config.h"  // ✅ Load settings from conf/config.conf
#include <Irc>
#include <IrcCommand>
#include <IrcMessage>
#include <QDebug>

// ✅ Constructor
IRCAPI::IRCAPI(QObject* parent)
    : QObject(parent), connection(new IrcConnection(this)) {
    connect(connection, &IrcConnection::connected, this, &IRCAPI::onConnected);
    connect(connection, &IrcConnection::disconnected, this, &IRCAPI::onDisconnected);
    connect(connection, &IrcConnection::messageReceived, this, &IRCAPI::onMessageReceived);

    // ✅ Load config from conf/config.conf
    server = QString::fromStdString(SERVER);
    port = PORT;
    useSSL = SSL_ENABLED;
    botNick = QString::fromStdString(BOT_NICK);
    botUser = QString::fromStdString(IDENT);
    botRealname = QString::fromStdString(GECOS);
    saslAccount = QString::fromStdString(SASL_ACCOUNT);
    saslPassword = QString::fromStdString(SASL_PASSWORD);
    channels = QString::fromStdString(CHANNELS).split(",", QString::SkipEmptyParts);
}

// ✅ Destructor
IRCAPI::~IRCAPI() {
    if (connection->isConnected()) {
        connection->disconnectFromHost();
    }
    delete connection;
}

// ✅ Connect to IRC server
void IRCAPI::connectToServer() {
    connection->setHost(server);
    connection->setPort(port);
    connection->setSecure(useSSL);
    connection->setNickName(botNick);
    connection->setUserName(botUser);
    connection->setRealName(botRealname);

    if (!saslAccount.isEmpty()) {
        connection->setSaslMechanism("PLAIN");
        connection->setSaslAccount(saslAccount);
        connection->setSaslPassword(saslPassword);
    }

    connection->open();
}

// ✅ Join channels from config
void IRCAPI::joinChannels() {
    for (const QString& channel : channels) {
        sendRawCommand("JOIN " + channel);
    }
}

// ✅ Send a message
void IRCAPI::sendMessage(const QString& target, const QString& message) {
    IrcCommand* cmd = IrcCommand::createMessage(target, message);
    connection->sendCommand(cmd);
}

// ✅ Send a raw IRC command
void IRCAPI::sendRawCommand(const QString& command) {
    IrcCommand* cmd = IrcCommand::createCustom(command);
    connection->sendCommand(cmd);
}

// ✅ Handle successful connection
void IRCAPI::onConnected() {
    qDebug() << "[IRC] Connected to server.";
    emit connected();
    joinChannels();  // ✅ Auto join channels after connecting
}

// ✅ Handle disconnection
void IRCAPI::onDisconnected() {
    qDebug() << "[IRC] Disconnected from server.";
    emit disconnected();
}

// ✅ Handle received messages
void IRCAPI::onMessageReceived(IrcMessage* message) {
    if (message->type() == IrcMessage::Private) {
        IrcPrivateMessage* privMsg = static_cast<IrcPrivateMessage*>(message);
        emit messageReceived(privMsg->nick(), privMsg->target(), privMsg->message());
    }
    emit rawMessageReceived(message->toString());
}

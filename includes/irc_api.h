#ifndef IRC_API_H
#define IRC_API_H

#include "IrcCore/ircglobal.h"
#include "IrcCore/ircconnection.h"
#include "IrcCore/irccommand.h"
#include "IrcCore/ircmessage.h"
#include "IrcModel/ircchannel.h"
#include "IrcUtil/irccommandqueue.h"
#include <QObject>
#include <QString>

class IRCClient : public QObject {
    Q_OBJECT

public:
    explicit IRCClient(QObject* parent = nullptr);
    ~IRCClient();

    void connectToServer();
    void run();
    void sendRaw(const QString& message);
    void joinChannels();
    void sendIrcMessage(const std::string& message);

signals:
    void disconnected();

private slots:
    void onConnected();
    void onDisconnected();
    void startSASLAuth();
    void sendSASLCredentials();
    void finalizeAuth();
    void onPrivateMessageReceived(IrcPrivateMessage* message);
    void rehash();
    void restart();

private:
    IrcConnection* connection;
};

// globally accessible function
extern IRCClient* global_irc_client;  
void send_irc_message(const std::string& message);

#endif // IRC_API_H

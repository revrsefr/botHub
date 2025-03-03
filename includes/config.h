#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <map>

// Global configuration variables
extern std::string SERVER;
extern int PORT;
extern bool SSL_ENABLED;
extern std::string BOT_NICK;
extern std::string IDENT;
extern std::string GECOS;
extern std::string SASL_ACCOUNT;
extern std::string SASL_PASSWORD;
extern std::string CHANNELS;
extern std::string GITHUB_API_KEY;
extern std::string DB_CONN;
extern std::map<std::string, std::string> IRC_COLORS;

void load_config();
std::map<std::string, std::string> read_config(const std::string& file_path);

#endif // CONFIG_H

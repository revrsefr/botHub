CXX = g++
MOC = moc
CXXFLAGS = -std=c++17 -Wall -fPIC -DIRC_SHARED -Iincludes -Ilibs/fmt/include -Ilibs/spdlog/include -Ilibs/loguru \
-I/usr/include/boost -I/usr/include -I/usr/include/x86_64-linux-gnu/qt5 -I/usr/include/x86_64-linux-gnu/qt5/QtCore \
-I/usr/include/x86_64-linux-gnu/qt5/QtNetwork -I/home/debian/irc/Bots/gitbot++/libs/libcommuni/include \
-I/home/debian/irc/Bots/gitbot++/libs/libcommuni/include/IrcCore -I/home/debian/irc/Bots/gitbot++/libs/libcommuni/include/IrcModel \
-I/home/debian/irc/Bots/gitbot++/libs/libcommuni/include/IrcUtil

LDFLAGS = -L/usr/lib -L/usr/lib/x86_64-linux-gnu -lpugixml -lfmt -lspdlog -lloguru -lpqxx -lpq -lboost_system -lpthread \
-lcpr -lssl -lcrypto -lcurl -lQt5Core -lQt5Network -lIrcCore -lIrcModel -lIrcUtil

HEADERS += includes/irc_api.h

SRC_DIR = src
MODULE_DIR = modules
UTILITY_DIR = utility
BIN_DIR = run

SRC_FILES = $(SRC_DIR)/main.cpp $(SRC_DIR)/config.cpp
MODULE_FILES = $(MODULE_DIR)/github.cpp $(MODULE_DIR)/database.cpp $(MODULE_DIR)/admin.cpp $(MODULE_DIR)/irc_client.cpp
UTILITY_FILES = $(UTILITY_DIR)/logger.cpp $(UTILITY_DIR)/helpers.cpp $(UTILITY_DIR)/base64.cpp

MOC_SOURCES = includes/irc_api.h
MOC_OUTPUT = includes/moc_irc_api.cpp
MOC_OBJECT = includes/moc_irc_api.o

OBJ_FILES = $(SRC_FILES:.cpp=.o) $(MODULE_FILES:.cpp=.o) $(UTILITY_FILES:.cpp=.o) $(MOC_OBJECT)
TARGET = $(BIN_DIR)/github-bot

all: build

build: $(TARGET)
	@echo "✅ Build complete!"

$(TARGET): $(OBJ_FILES)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ_FILES) $(LDFLAGS)

$(MOC_OUTPUT): $(MOC_SOURCES)
	$(MOC) $< -o $@

$(MOC_OBJECT): $(MOC_OUTPUT)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(SRC_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(MODULE_DIR)/%.o: $(MODULE_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(UTILITY_DIR)/%.o: $(UTILITY_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(SRC_DIR)/*.o $(MODULE_DIR)/*.o $(UTILITY_DIR)/*.o $(MOC_OUTPUT) $(MOC_OBJECT)
	@echo "🧹 Cleaned up build files!"

rebuild: clean all

# Variables
COMPILER = g++
FLAGS = -c -Wall -Wextra -Wcast-align -Wwrite-strings -Waggregate-return -O2 -std=c++17
LIBS = -lgdi32 -luser32 -lws2_32 -liphlpapi -I"./HEADER" -I"C:/Program Files/OpenSSL-Win64/include" -L"C:/Program Files/OpenSSL-Win64/lib/VC/x64/MT" -lssl -lcrypto -fpermissive

SRC_DIR = ./SRC
HEADER_DIR = ./HEADER
OBJ_DIR = ./BUILD
OUTPUT = ./BIN/demo.exe

# Find all .cpp files in SRC_DIR
SRC_FILES = $(wildcard $(SRC_DIR)/**/*.cpp) $(wildcard $(SRC_DIR)/*.cpp)

# Convert .cpp files to .o files
OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Default rule
all: $(OUTPUT)

build: $(OUTPUT)

# Link object files into executable
$(OUTPUT): $(OBJ_FILES)
	@echo "Linking objects to executable..."
	@mkdir -p $(dir $@)
	@$(COMPILER) $(OBJ_FILES) -o $@ $(LIBS)
	@echo "Build completed successfully."

# Compile each .cpp file into .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	@$(COMPILER) $(FLAGS) $< -o $@ $(LIBS)

# Run the program
run:
	@$(OUTPUT)

# Clean build files
clean:
	@rm -rf $(OBJ_DIR) $(OUTPUT)
	@echo "Cleaned build files."

# Dependency rules (optional but improves clarity)
$(OBJ_DIR)/SERVER/ProxyServer.o: $(HEADER_DIR)/ProxyServer.hpp $(HEADER_DIR)/ClientHandler.hpp
$(OBJ_DIR)/SERVER/Logger.o: $(HEADER_DIR)/Setting.hpp $(HEADER_DIR)/Logger.hpp
$(OBJ_DIR)/BLOCKURL/BlackList.o: $(HEADER_DIR)/BlackList.hpp
$(OBJ_DIR)/HANDLE/ClientHandler.o: $(HEADER_DIR)/ClientHandler.hpp $(HEADER_DIR)/BlackList.hpp $(HEADER_DIR)/Logger.hpp
$(OBJ_DIR)/HANDLE/HttpHandler.o: $(HEADER_DIR)/HttpHandler.hpp $(HEADER_DIR)/Logger.hpp
$(OBJ_DIR)/HANDLE/Request.o: $(HEADER_DIR)/Request.hpp $(HEADER_DIR)/Setting.hpp $(HEADER_DIR)/HttpHandler.hpp $(HEADER_DIR)/Logger.hpp
$(OBJ_DIR)/HANDLE/Response.o: $(HEADER_DIR)/Response.hpp $(HEADER_DIR)/Setting.hpp $(HEADER_DIR)/HttpHandler.hpp $(HEADER_DIR)/Logger.hpp
$(OBJ_DIR)/HANDLE/SocketHandler.o: $(HEADER_DIR)/SocketHandler.hpp $(HEADER_DIR)/Logger.hpp
$(OBJ_DIR)/NETWORK/NetworkManager.o: $(HEADER_DIR)/NetworkManager.hpp $(HEADER_DIR)/Logger.hpp
$(OBJ_DIR)/THREAD/ThreadManager.o: $(HEADER_DIR)/ThreadManager.hpp $(HEADER_DIR)/Logger.hpp
$(OBJ_DIR)/THREAD/ThreadPool.o: $(HEADER_DIR)/ThreadPool.hpp $(HEADER_DIR)/ClientHandler.hpp
$(OBJ_DIR)/main.o: $(HEADER_DIR)/Setting.hpp $(HEADER_DIR)/ProxyServer.hpp

.PHONY: all clean run build
# Biến số
COMPILER = g++
FLAGS = -c -Wall -Wextra -Wcast-align -Wwrite-strings -Waggregate-return -O2
LIBS = -lgdi32 -luser32 -lws2_32 -liphlpapi -I"C:/Program Files/OpenSSL-Win64/include" -L"C:/Program Files/OpenSSL-Win64/lib/VC/x64/MT" -lssl -lcrypto -fpermissive

SRC_DIR = ./SRC
OBJ_DIR = ./BIN
OUTPUT = ./BIN/demo.exe

# Tìm tất cả các file .cpp trong SRC_DIR
SRC_FILES = $(wildcard $(SRC_DIR)/**/*.cpp) $(wildcard $(SRC_DIR)/*.cpp)

# Chuyển đổi file .cpp thành file .o tương ứng
OBJ_FILES = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Quy tắc mặc định
all: $(OUTPUT)

# Liên kết các file đối tượng thành file thực thi
$(OUTPUT): $(OBJ_FILES)
	@echo "Linking objects to executable..."
	$(COMPILER) $(OBJ_FILES) -o $@ $(LIBS)
	@echo "Built successfully."

# Biên dịch từng file .cpp thành file .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	$(COMPILER) $(FLAGS) $< -o $@ $(LIBS)

# Tạo thư mục obj_dir nếu chưa tồn tại
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

# Xóa các file build
clean:
	@rm -rf $(OBJ_DIR) $(OUTPUT)
	@echo "Cleaned build files."

.PHONY: all clean

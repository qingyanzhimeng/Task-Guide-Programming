
# 编译器
CXX = g++

# 编译选项
CXXFLAGS = -Wall -g -std=c++11

# 生成的可执行文件名
TARGET = server

# 源文件
SRC = main.cpp

# 默认目标
all: $(TARGET)

# 编译生成可执行文件
$(TARGET): $(SRC) tcp_server.hpp
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

# 清理编译文件
clean:
	rm -f $(TARGET)

.PHONY: all clean
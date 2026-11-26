# Makefile
CC = g++
CXXFLAGS = -Wall -g -O2 -std=c++14
LDFLAGS = -pthread

# 目标程序名
TARGET = tcp_server

# 源文件列表
SRCS = main.cpp tcp_server.cpp thread_pool.cpp

all: $(TARGET)

# 直接一次性编译所有源码，不生成 .o
$(TARGET): $(SRCS)
	$(CC) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

# 清理
clean:
	rm -f $(TARGET)

.PHONY: all clean

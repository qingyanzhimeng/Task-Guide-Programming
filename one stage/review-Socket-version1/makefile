# 目标可执行文件
TARGET = server
# 源码列表
SRC = main.cpp tcp_server.cpp
# 编译器
CXX = g++
CXXFLAGS = -Wall -g -std=c++11
# 链接选项，pthread放这里
LDFLAGS = -pthread

# 编译链接规则
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

# 清理
clean:
	rm -f $(TARGET) *.o

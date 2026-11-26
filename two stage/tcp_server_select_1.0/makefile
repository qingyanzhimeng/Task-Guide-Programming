# 目标可执行文件名
TARGET = tcp_server_select

# 源文件
SRCS = main.cpp tcp_server.cpp
# 自动推导目标文件
OBJS = $(SRCS:.cpp=.o)

# 编译器
CXX = g++
# 编译参数：C++17，警告，-pthread
CXXFLAGS = -std=c++17 -Wall -Wextra
# 链接参数，必须加 -pthread
LDFLAGS = -pthread

# 最终链接
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

# 编译cpp到o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean

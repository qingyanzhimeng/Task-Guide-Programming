# 可执行文件名
TARGET := tcp_server
# 源文件列表
SRCS := main.cpp tcp_server.cpp thread_pool.cpp
# 编译器
CXX := g++
# 编译参数：C++17标准 + 开启警告 + 调试信息
CXXFLAGS := -std=c++17 -Wall -g
# 链接参数：pthread 线程库
LDFLAGS := -pthread

# 自动推导obj文件
OBJS := $(SRCS:.cpp=.o)

# 终极目标
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

# 生成.o文件规则
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理
clean:
	rm -f $(OBJS) $(TARGET)

# 伪目标
.PHONY: clean

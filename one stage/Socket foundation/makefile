# 最终可执行程序名
TARGET = server

# 源文件
SRC = main.cpp

# 编译器，C++用g++
CXX = g++
CXXFLAGS = -Wall -g -std=c++11

# 链接目标
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

# 清理
clean:
	rm -f $(TARGET) *.o

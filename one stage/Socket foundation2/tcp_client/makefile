CXX = g++
CXXFLAGS = -Wall -g -std=c++11
TARGET = tcp_client
SRC = main.cpp tcp_client.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)

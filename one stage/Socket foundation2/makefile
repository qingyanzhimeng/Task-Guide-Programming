CXX = g++
CXXFLAGS = -Wall -g

TARGET = server
OBJS = main.o tcp_server.o

all: $(TARGET)
	rm -f $(OBJS)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

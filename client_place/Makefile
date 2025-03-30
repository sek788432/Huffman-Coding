CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I..
LDFLAGS = -pthread

TARGET = client
SRCS = client.cpp ../utils.cpp
OBJS = $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) compressed.txt decoded.txt code.txt for_decompressed.txt decoded.jpg compressed.jpg decoded.jpeg compressed.jpeg *log *o

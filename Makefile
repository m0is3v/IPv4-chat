CC = g++
CXXFLAGS = -std=c++17 -pthread -Wall -Wextra
TARGET = main
SOURCES = main.cpp ipv4chat.cpp
HEADERS = ipv4chat.h

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CXXFLAGS) -o $(TARGET) $(SOURCES)
clean:
	rm -f $(TARGET)

.PHONY: all clean

CC = g++
CFLAGS = -std=c++17 -pthread
TARGET = chat
#
all: $(TARGET)

$(TARGET): main.cpp
	$(CC) $(CFLAGS) -o $(TARGET) main.cpp

clean:
	rm -f $(TARGET)

.PHONY: all clean

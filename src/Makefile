CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra -O2 -I.
LDFLAGS = -lcpprest -lpthread -lssl -lcrypto
SOURCES = server.cpp
TARGET = servidor

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: run clean
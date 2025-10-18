CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra -O2 -I.
LDFLAGS = -lcpprest -lpthread -lssl -lcrypto
TARGET = servidor

# Compila APENAS server.cpp (se ele inclui web.cpp)
$(TARGET): server.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) server.cpp $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: run clean
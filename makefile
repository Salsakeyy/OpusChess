# Makefile for Simple Chess Engine

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -march=native -flto
LDFLAGS = -pthread -flto

# Source files
SOURCES = src/main.cpp \
          src/board.cpp \
          src/move.cpp \
          src/movegen.cpp \
          src/evaluation.cpp \
          src/search.cpp \
          src/uci.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Executable names
EXECUTABLE = chess_engine
TEST_EXECUTABLE = perft_test

# Test program sources
TEST_SOURCES = src/perft_test.cpp \
               src/board.cpp \
               src/move.cpp \
               src/movegen.cpp

TEST_OBJECTS = $(TEST_SOURCES:.cpp=.o)

# Default target
all: $(EXECUTABLE)

# Build the executable
$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

# Build the test program
test: $(TEST_EXECUTABLE)

$(TEST_EXECUTABLE): $(TEST_OBJECTS)
	$(CXX) $(TEST_OBJECTS) -o $@ $(LDFLAGS)

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TEST_OBJECTS) $(EXECUTABLE) $(TEST_EXECUTABLE)

# Debug build
debug: CXXFLAGS = -std=c++17 -Wall -Wextra -g -O0 -DDEBUG
debug: clean all

# Profile build
profile: CXXFLAGS += -pg
profile: LDFLAGS += -pg
profile: clean all

.PHONY: all clean debug profile test
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
TARGET ?= chess_engine

# Test program sources
TEST_SOURCES = src/perft_test.cpp \
               src/board.cpp \
               src/move.cpp \
               src/movegen.cpp

TEST_OBJECTS = $(TEST_SOURCES:.cpp=.o)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

# Legacy target for backwards compatibility
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
	rm -f $(OBJECTS) $(TEST_OBJECTS) $(EXECUTABLE) $(TEST_EXECUTABLE) $(TEST_ENGINE)
	rm -f $(BASE_ENGINE)_prev $(TARGET)_prev
	rm -f chess_engine_prev chess_engine_test
# Debug build
debug: CXXFLAGS = -std=c++17 -Wall -Wextra -g -O0 -DDEBUG
debug: clean all

# Profile build
profile: CXXFLAGS += -pg
profile: LDFLAGS += -pg
profile: clean all


# Add these targets to your existing Makefile

# Testing variables
TEST_DIR = tests
TEST_SCRIPT = $(TEST_DIR)/engine_test.py
SIMPLE_TEST = $(TEST_DIR)/simple_test.py
BASE_ENGINE = $(TARGET)
TEST_ENGINE = $(TARGET)_test

# Number of test games
TEST_GAMES ?= 100
TEST_CONCURRENCY ?= 1
TEST_TIME ?= 100
TEST_INC ?= 10

# Build test version
test-build: 
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -DTEST_VERSION" TARGET=$(TEST_ENGINE)

# Run simple test (no external dependencies)
test-simple: $(TARGET) test-build
	@echo "Running simple engine test..."
	@python3 $(SIMPLE_TEST) $(BASE_ENGINE) $(TEST_ENGINE) \
		--games $(TEST_GAMES) \
		--concurrency $(TEST_CONCURRENCY) \
		--time $(TEST_TIME) \
		--inc $(TEST_INC)

# Run full test (requires cutechess-cli)
test-full: $(TARGET) test-build
	@echo "Running full engine test with SPRT..."
	@python3 $(TEST_SCRIPT) $(BASE_ENGINE) $(TEST_ENGINE) \
		--games $(TEST_GAMES) \
		--concurrency $(TEST_CONCURRENCY) \
		--tc "10+0.1" \
		--sprt-elo0 -1.75 \
		--sprt-elo1 0.25

# Quick test for development
test-quick: $(TARGET) test-build
	@echo "Running quick test (20 games)..."
	@python3 $(SIMPLE_TEST) $(BASE_ENGINE) $(TEST_ENGINE) \
		--games 20 \
		--concurrency 1 \
		--time 1000 \
		--inc 10

# Run perft test
test-perft: $(TARGET)
	@echo "Running perft tests..."
	@./$(TARGET)_perft

# Clean test files
clean-test:
	rm -f $(TEST_ENGINE)
	rm -f test_*.json
	rm -f $(TARGET)_perft

# Setup test directory
setup-test:
	@mkdir -p $(TEST_DIR)
	@echo "Creating test scripts..."
	@# Copy the Python test scripts to the test directory
	@echo "Test directory setup complete."

# Test against previous version (assumes git)
test-regression: $(TARGET)
	@echo "Building previous version..."
	@git stash
	@$(MAKE) clean
	@$(MAKE) TARGET=$(BASE_ENGINE)_prev
	@cp $(BASE_ENGINE)_prev $(BASE_ENGINE)_prev.backup
	@git stash pop
	@$(MAKE) clean
	@$(MAKE) TARGET=$(TEST_ENGINE)
	@mv $(BASE_ENGINE)_prev.backup $(BASE_ENGINE)_prev
	@echo "Testing current vs previous version..."
	@python3 $(SIMPLE_TEST) $(BASE_ENGINE)_prev $(TEST_ENGINE) \
		--games $(TEST_GAMES) \
		--concurrency $(TEST_CONCURRENCY)
	@rm -f $(BASE_ENGINE)_prev
# Help for testing
help-test:
	@echo "Testing targets:"
	@echo "  make test-simple    - Run simple test (no external dependencies)"
	@echo "  make test-full      - Run full SPRT test (requires cutechess-cli)"
	@echo "  make test-quick     - Quick 20-game test for development"
	@echo "  make test-perft     - Run move generation tests"
	@echo "  make test-regression - Test against previous git version"
	@echo ""
	@echo "Variables:"
	@echo "  TEST_GAMES=N       - Number of games (default: 100)"
	@echo "  TEST_CONCURRENCY=N - Concurrent games (default: 1)"
	@echo "  TEST_TIME=MS       - Time per move in ms (default: 1000)"
	@echo ""
	@echo "Example:"
	@echo "  make test-simple TEST_GAMES=1000 TEST_CONCURRENCY=4"

.PHONY: test-build test-simple test-full test-quick test-perft clean-test setup-test test-regression help-test

# Makefile for Simple Chess Engine

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -march=native -flto
LDFLAGS = -pthread -flto

# Configurable evaluation source
EVAL_SOURCE ?= src/evaluation.cpp
SEARCH_SOURCE ?= src/search.cpp

# Source files
SOURCES = src/main.cpp \
          src/board.cpp \
          src/move.cpp \
          src/movegen.cpp \
          $(EVAL_SOURCE) \
          $(SEARCH_SOURCE) \
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
	rm -f chess_engine_prev chess_engine_test chess_engine_noeval
	rm -f chess_engine_eval1 chess_engine_eval2

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
TEST_GAMES ?= 1000
TEST_CONCURRENCY ?= 1
TEST_TIME ?= 10000
TEST_INC ?= 100

# Build test version
test-build: 
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -DTEST_VERSION" TARGET=$(TEST_ENGINE)

# Build version without evaluation (material only)
build-noeval:
	$(MAKE) clean
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -DNO_EVAL" TARGET=chess_engine_noeval

# Build normal version with evaluation
build-eval:
	$(MAKE) clean
	$(MAKE) TARGET=chess_engine_test

# Build engine with evaluation.cpp
build-eval1:
	$(MAKE) clean
	$(MAKE) EVAL_SOURCE=src/evaluation.cpp TARGET=chess_engine_eval1

# Build engine with evaluation2.cpp
build-eval2:
	$(MAKE) clean
	$(MAKE) EVAL_SOURCE=src/evaluation2.cpp TARGET=chess_engine_eval2
	$(MAKE) SEARCH_SOURCE=src/search2.cpp TARGET=chess_engine_eval2

# Test evaluation.cpp vs evaluation2.cpp
test-eval: 
	@echo "Building engine with evaluation.cpp..."
	$(MAKE) clean
	$(MAKE) EVAL_SOURCE=src/evaluation.cpp TARGET=chess_engine_eval1
	@cp chess_engine_eval1 chess_engine_eval1.tmp
	@echo "Building engine with evaluation2.cpp..."
	$(MAKE) clean
	$(MAKE) EVAL_SOURCE=src/evaluation2.cpp TARGET=chess_engine_eval2
	@mv chess_engine_eval1.tmp chess_engine_eval1
	@echo "Testing evaluation.cpp vs evaluation2.cpp..."
	@python3 $(SIMPLE_TEST) chess_engine_eval1 chess_engine_eval2 \
		--games $(TEST_GAMES) \
		--concurrency $(TEST_CONCURRENCY) \
		--time $(TEST_TIME) \
		--inc $(TEST_INC)

# Test evaluation vs no evaluation (renamed from original test-eval)
test-eval-vs-noeval: 
	@echo "Building no-eval engine..."
	$(MAKE) clean
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -DNO_EVAL" TARGET=chess_engine_noeval
	@cp chess_engine_noeval chess_engine_noeval.tmp
	@echo "Building evaluation engine..."
	$(MAKE) clean
	$(MAKE) TARGET=chess_engine_test
	@mv chess_engine_noeval.tmp chess_engine_noeval
	@echo "Testing evaluation effectiveness..."
	@python3 $(SIMPLE_TEST) chess_engine_noeval chess_engine_test \
		--games $(TEST_GAMES) \
		--concurrency $(TEST_CONCURRENCY) \
		--time $(TEST_TIME) \
		--inc $(TEST_INC)

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
		--games 1000 \
		--concurrency 4 \
		--time 10000 \
		--inc 100

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
	@echo "  make test-simple         - Run simple test (no external dependencies)"
	@echo "  make test-full           - Run full SPRT test (requires cutechess-cli)"
	@echo "  make test-quick          - Quick 20-game test for development"
	@echo "  make test-perft          - Run move generation tests"
	@echo "  make test-regression     - Test against previous git version"
	@echo "  make test-eval           - Test evaluation.cpp vs evaluation2.cpp"
	@echo "  make test-eval-vs-noeval - Test evaluation vs material-only engine"
	@echo ""
	@echo "Build targets:"
	@echo "  make build-eval1         - Build engine with evaluation.cpp"
	@echo "  make build-eval2         - Build engine with evaluation2.cpp"
	@echo "  make build-noeval        - Build engine without evaluation"
	@echo ""
	@echo "Variables:"
	@echo "  TEST_GAMES=N       - Number of games (default: 100)"
	@echo "  TEST_CONCURRENCY=N - Concurrent games (default: 1)"
	@echo "  TEST_TIME=MS       - Time per move in ms (default: 1000)"
	@echo "  EVAL_SOURCE=file   - Evaluation source file (default: src/evaluation.cpp)"
	@echo ""
	@echo "Example:"
	@echo "  make test-simple TEST_GAMES=1000 TEST_CONCURRENCY=4"
	@echo "  make build-eval1"
	@echo "  make test-eval TEST_GAMES=500"

.PHONY: test-build test-simple test-full test-quick test-perft clean-test setup-test test-regression help-test build-noeval build-eval test-eval build-eval1 build-eval2 test-eval-vs-noeval
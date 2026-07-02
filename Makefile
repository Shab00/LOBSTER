CC = clang
CFLAGS = -O3 -march=native -Wall -Wextra -fPIC -std=c11
LDFLAGS = -lm -lcjson

# macOS ARM64 homebrew paths
CJSON_INCLUDE = -I/opt/homebrew/include
CJSON_LIB = -L/opt/homebrew/lib

SRCDIR = src
INCDIR = include
TESTDIR = tests
OBJDIR = build
BINDIR = bin

# Core library sources (no main)
LIB_SOURCES := $(filter-out $(SRCDIR)/%_main.c, $(wildcard $(SRCDIR)/*.c))
LIB_OBJECTS := $(LIB_SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Programs with main
MAIN_SOURCES := $(wildcard $(SRCDIR)/*_main.c)
MAIN_BINS := $(MAIN_SOURCES:$(SRCDIR)/%_main.c=$(BINDIR)/%)

# Auto-discover all test files
TEST_SOURCES := $(wildcard $(TESTDIR)/test_*.c)
TEST_BINS := $(TEST_SOURCES:$(TESTDIR)/test_%.c=$(BINDIR)/test_%)

# Default target: build all test binaries
all: $(TEST_BINS) $(MAIN_BINS)

# Ensure directories exist FIRST
$(OBJDIR) $(BINDIR):
	mkdir -p $@

# Compile core library sources to object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $(CJSON_INCLUDE) -c $< -o $@

# Link test binaries (only use core library objects, not main programs)
$(BINDIR)/test_%: $(TESTDIR)/test_%.c $(LIB_OBJECTS) | $(BINDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $(CJSON_INCLUDE) $(LIB_OBJECTS) $< -o $@ $(CJSON_LIB) $(LDFLAGS)

# Link main programs
$(BINDIR)/%: $(SRCDIR)/%_main.c $(LIB_OBJECTS) | $(BINDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $(CJSON_INCLUDE) $(LIB_OBJECTS) $< -o $@ $(CJSON_LIB) $(LDFLAGS)

# Run all tests
run_tests: $(TEST_BINS)
	@for test in $(TEST_BINS); do \
		echo "=== Running $$test ==="; \
		$$test || exit 1; \
		echo; \
	done

# Run a specific test: make run_test TEST=simulator
run_test: $(BINDIR)/test_$(TEST)
	./$(BINDIR)/test_$(TEST)

# Run a benchmark: make run BENCH=benchmark
run: $(BINDIR)/$(BENCH)
	./$(BINDIR)/$(BENCH)

# Clean build artifacts
clean:
	rm -rf $(OBJDIR) $(BINDIR)

# Verbose output
verbose: CFLAGS += -DDEBUG
verbose: clean all

.PHONY: all run_tests run_test run clean verbose

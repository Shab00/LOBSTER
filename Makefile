CC = clang
CFLAGS = -O3 -march=native -Wall -Wextra -fPIC -std=c11
LDFLAGS = -lm -lcjson

# Auto-detect Homebrew prefix (works on both Intel and ARM Macs)
BREW_PREFIX = $(shell brew --prefix)

CJSON_INCLUDE = -I$(BREW_PREFIX)/include
CJSON_LIB = -L$(BREW_PREFIX)/lib

# libwebsockets + OpenSSL
LWS_CFLAGS = $(shell pkg-config --cflags libwebsockets) -I$(BREW_PREFIX)/opt/openssl@3/include
LWS_LIBS = $(shell pkg-config --libs libwebsockets) -L$(BREW_PREFIX)/opt/openssl@3/lib

SRCDIR = src
INCDIR = include
TESTDIR = tests
OBJDIR = build
BINDIR = bin

# All .c files
ALL_SOURCES = $(wildcard $(SRCDIR)/*.c)
TEST_SOURCES = $(wildcard $(TESTDIR)/*.c)

# Anything with _main.c or in tests/ is a standalone program.
# Everything else in src/ is a library object.
LIB_SOURCES = $(filter-out $(SRCDIR)/%_main.c, $(ALL_SOURCES))
LIB_OBJECTS = $(LIB_SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Standalone programs: src/*_main.c -> bin/*
MAIN_SOURCES = $(filter $(SRCDIR)/%_main.c, $(ALL_SOURCES))
MAIN_BINS = $(MAIN_SOURCES:$(SRCDIR)/%_main.c=$(BINDIR)/%)

# Standalone programs: tests/*.c -> bin/test_*
TEST_BINS = $(TEST_SOURCES:$(TESTDIR)/%.c=$(BINDIR)/%)

# Everything
all: $(MAIN_BINS) $(TEST_BINS)

# Directories
$(OBJDIR) $(BINDIR):
	mkdir -p $@

# Compile library sources (files in src/ that are NOT *_main.c)
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $(CJSON_INCLUDE) $(LWS_CFLAGS) -c $< -o $@

# Link src/*_main.c programs (link against library objects)
$(BINDIR)/%: $(SRCDIR)/%_main.c $(LIB_OBJECTS) | $(BINDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $(CJSON_INCLUDE) $(LWS_CFLAGS) $< $(LIB_OBJECTS) -o $@ $(CJSON_LIB) $(LDFLAGS) $(LWS_LIBS)

# Link tests/*.c programs (link against library objects)
$(BINDIR)/%: $(TESTDIR)/%.c $(LIB_OBJECTS) | $(BINDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) $(CJSON_INCLUDE) $(LWS_CFLAGS) $< $(LIB_OBJECTS) -o $@ $(CJSON_LIB) $(LDFLAGS) $(LWS_LIBS)

# Build shared library for Python bindings
$(BINDIR)/liblobster.dylib: $(LIB_OBJECTS) | $(BINDIR)
	$(CC) -shared -o $@ $(LIB_OBJECTS) $(CJSON_LIB) $(LDFLAGS)

lib: $(BINDIR)/liblobster.dylib

# Run all tests
test: $(TEST_BINS)
	@passed=0; failed=0; \
	for test in $(TEST_BINS); do \
		echo "=== $$test ==="; \
		if $$test; then passed=$$((passed+1)); else failed=$$((failed+1)); fi; \
		echo; \
	done; \
	echo "Passed: $$passed | Failed: $$failed"

# Run all tests + all mains (verify they start)
check: test $(MAIN_BINS)
	@for prog in $(MAIN_BINS); do echo "=== $$prog --help ==="; $$prog --help 2>&1 | head -3; echo; done

# Run a specific binary: make run BIN=ws_main
run: $(BINDIR)/$(BIN)
	./$(BINDIR)/$(BIN)

# Clean
clean:
	rm -rf $(OBJDIR) $(BINDIR)

# Debug build
debug: CFLAGS = -g -O0 -Wall -Wextra -fPIC -std=c11 -DDEBUG
debug: clean all

.PHONY: all test check run clean debug lib

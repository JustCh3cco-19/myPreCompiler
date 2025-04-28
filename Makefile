# Compiler and flags
CC = gcc
CFLAGS = -Wall -Werror -g -Iinclude

# Directories
SRC_DIR = src
INCLUDE_DIR = include
TEST_DIR = test
OBJ_DIR = obj

# Target executable
TARGET = myPreCompiler

# Source files
SOURCES = $(SRC_DIR)/main.c $(SRC_DIR)/preprocessor.c $(SRC_DIR)/fileutils.c $(SRC_DIR)/statistics.c

# Object files
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Header files
HEADERS = $(INCLUDE_DIR)/preprocessor.h $(INCLUDE_DIR)/fileutils.h $(INCLUDE_DIR)/statistics.h

# Default target
all: $(OBJ_DIR) $(TARGET)

# Create object directory
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Linking rule
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# Compilation rule
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(TARGET) $(OBJECTS)
	rmdir $(OBJ_DIR) 2>/dev/null || true

# Test rule with statistics enabled
test: $(TARGET)
	@echo "Eseguo il test in $(TEST_DIR)..."
	@cd $(TEST_DIR) && ../$(TARGET) -v -s test_input.c -o test_output.c
	@if [ ! -f $(TEST_DIR)/expected_output.c ]; then \
	  echo "Generating expected_output.c..."; \
	  cp $(TEST_DIR)/test_output.c $(TEST_DIR)/expected_output.c; \
	fi
	@echo "Confronto con expected_output.c..."
	@if diff -q $(TEST_DIR)/test_output.c $(TEST_DIR)/expected_output.c >/dev/null; then \
	    echo "✓ Test passed!"; \
	else \
	    echo "✗ Test failed!"; \
	    exit 1; \
	fi

# Run with statistics
stats: $(TARGET)
	@echo "Esecuzione con statistiche..."
	./$(TARGET) -v -s $(FILE_INPUT) -o $(FILE_OUTPUT)

# Phony targets
.PHONY: all clean test stats
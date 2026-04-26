# Author: Team Hikaru Naka-more
# Date: April 19, 2026

# Compiler settings
CC = gcc
GTK_CFLAGS = `pkg-config --cflags gtk+-3.0`
GTK_LIBS = `pkg-config --libs gtk+-3.0`

# Inject GTK headers into standard compilation flags
CFLAGS = -Wall -std=c99 -g $(GTK_CFLAGS)

# Directory Definitions
SRC_DIR = src
BIN_DIR = bin
DOC_DIR = doc

# Target executables
TARGET = $(BIN_DIR)/chess
TEST_BOARD = $(BIN_DIR)/test_boarddisplay
TEST_RULES = $(BIN_DIR)/test_rulecheck

# Core modules (excluding main.o to prevent duplicate main definitions)
CORE_OBJS = $(SRC_DIR)/GameData.o $(SRC_DIR)/MoveValidation.o \
            $(SRC_DIR)/MoveList.o $(SRC_DIR)/ChessAI.o $(SRC_DIR)/GUI.o

# Main program objects
OBJS = $(SRC_DIR)/main.o $(CORE_OBJS)

# Default target: builds the main game 
all: $(TARGET)

# Ensure the binary directory exists
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Link the main executable with GTK libraries
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(GTK_LIBS)

$(TEST_BOARD): $(SRC_DIR)/test_boarddisplay.o $(CORE_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(TEST_BOARD) $(SRC_DIR)/test_boarddisplay.o $(CORE_OBJS) $(GTK_LIBS)

$(TEST_RULES): $(SRC_DIR)/test_rulecheck.o $(CORE_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(TEST_RULES) $(SRC_DIR)/test_rulecheck.o $(CORE_OBJS) $(GTK_LIBS)

# Mandatory test target 
# Executes unit tests and brings up the initial board via the display test
test: $(TEST_BOARD) $(TEST_RULES)
	@echo "--- Running Rules Engine Validation ---"
	./$(TEST_RULES)
	@echo "--- Running Board Display Validation ---"
	./$(TEST_BOARD)

# Compile object files from the src directory
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Mandatory tar target for source code package
# Creates a self-contained directory to match INSTALL instructions
tar: clean all
	mkdir -p Chess_Alpha_src/bin
	mkdir -p Chess_Alpha_src/doc
	mkdir -p Chess_Alpha_src/resources
	cp -r $(SRC_DIR) Makefile Chess_Alpha_src/
	cp -r resources/* Chess_Alpha_src/resources/
	cp README* INSTALL* COPYRIGHT* Chess_Alpha_src/
	cp $(DOC_DIR)/Chess_UserManual.pdf $(DOC_DIR)/Chess_SoftwareSpec.pdf Chess_Alpha_src/doc/
	cp $(TARGET) Chess_Alpha_src/bin/
	tar -cvzf Chess_Alpha_src.tar.gz Chess_Alpha_src
	rm -rf Chess_Alpha_src

# Mandatory clean target 
clean:
	rm -rf $(SRC_DIR)/*.o $(BIN_DIR)/* Chess_Alpha_src.tar.gz

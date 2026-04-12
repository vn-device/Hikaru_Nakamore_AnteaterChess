# Compiler settings
CC = gcc
CFLAGS = -Wall -ansi -std=c99

# Target executable
TARGET = chess

# Source and Object files
SRCS = main.c GameData.c MoveValidation.c MoveList.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
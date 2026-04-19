# Compiler settings
CC = gcc
CFLAGS = -Wall -ansi -std=c99

# Target executable
TARGET = chess

# Source and Object files
SRCS = main.c GameData.c MoveValidation.c MoveList.c ChessAI.c
HDRS = GameData.h MoveValidation.h MoveList.h ChessAI.h
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

tar:
	tar -czf Hikaru_Nakamore_AnteaterChess-src.tar.gz $(SRCS) $(HDRS) Makefile LICENSE

clean:
	rm -f $(OBJS) $(TARGET)
.PHONY: all clean

CC = g++
CFLAGS = -std=c++17

all: clean
	flex scanner.lex
	$(CC) $(CFLAGS) -o hw1 *.c *.cpp
clean:
	rm -f lex.yy.c hw1

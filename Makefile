CC = gcc
CFLAGS = -Wall -Wextra -std=c11

wordsearch: wordsearch.c
	$(CC) $(CFLAGS) -o wordsearch wordsearch.c

clean:
	rm -f wordsearch

.PHONY: clean


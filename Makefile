CC     := gcc
CFLAGS := -Wall -Wextra -std=c99
SRC    := $(wildcard src/*.c)

.PHONY: run clean

bombgame: $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

run: bombgame
	./bombgame

clean:
	rm -f bombgame

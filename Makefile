CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g

TARGETS = server client \

all: $(TARGETS)

server: tcp_servidor.c
	$(CC) $(CFLAGS) -o $@ $<

client: tcp_cliente.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean

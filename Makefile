CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g

TARGETS = tcp_servidor tcp_cliente \

all: $(TARGETS)

tcp_servidor: tcp_servidor.c
	$(CC) $(CFLAGS) -o $@ $<

tcp_cliente: tcp_cliente.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean

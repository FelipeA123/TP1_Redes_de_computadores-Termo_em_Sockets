CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g

TARGETS = tcp_servidor tcp_cliente \
          udp_servidor udp_cliente \
          local_servidor local_cliente \
          raw_ping

all: $(TARGETS)

copy-tcp_servidor: tcp_servidor.c
	$(CC) $(CFLAGS) -o $@ $<

copy-tcp_cliente: tcp_cliente.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean

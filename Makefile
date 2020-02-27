CC=gcc

CFLAGS_SERVER=-I.
LIBS_SERVER=

CFLAGS_CLIENT=-I.
LIBS_CLIENT=-lX11 -lXi

all: server client

%.os: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS_SERVER)

%.oc: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS_CLIENT)

server: server.os
	$(CC) -o server server.os $(LIBS_SERVER)

client: client.oc
	$(CC) -o client client.oc $(LIBS_CLIENT)

.PHONY: clean

clean:
	rm -f server client *.os *.oc
CC=gcc

CFLAGS_PRODUCER=-I.
LIBS_PRODUCER=

CFLAGS_CONSUMER=-I.
LIBS_CONSUMER=

all: producer consumer

%.os: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS_PRODUCER)

%.oc: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS_CONSUMER)

producer: producer.os
	$(CC) -o producer producer.os $(LIBS_PRODUCER)

consumer: consumer.oc
	$(CC) -o consumer consumer.oc $(LIBS_CONSUMER)

.PHONY: clean

clean:
	rm -f producer consumer *.os *.oc

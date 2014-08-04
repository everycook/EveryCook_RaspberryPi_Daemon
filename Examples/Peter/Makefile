OBJECTS=ad7794_interface.o
BIN=ad7794_interface
CFLAGS=-g

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

ad7794_interface: $(OBJECTS)

all: $(BIN)

test: ad7794_interface
	./ad7794_interface

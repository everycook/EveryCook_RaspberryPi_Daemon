#DEBUG	= -g -O0
DEBUG	= -O3
CC	= gcc
INCLUDE	= -I/usr/local/include
CFLAGS	= $(DEBUG) -Wall $(INCLUDE) -Winline -pipe

LDFLAGS	= -L/usr/local/lib
LIBS	= -lwiringPi -lpthread

SRC 	=	eccontroller.c virtualspi.c virtuali2c.c gpio.c
OBJ	=	eccontroller.o virtualspi.o virtuali2c.o gpio.o
BINS	=	eccontroller virtualspi virtuali2c gpio

all:	eccontroller.o
	@echo [link]
	$(CC) -o $@ eccontroller.o $(LDFLAGS) $(LIBS)
virtualspi: virtualspi.o
	@echo [link]
	$(CC) -o $@ virtualspi.o $(LDFLAGS) $(LIBS)
virtuali2c: virtuali2c.o
	@echo [link]
	$(CC) -o $@ virtuali2c.o $(LDFLAGS) $(LIBS)
gpio: gpio.o
	@echo [link]
	$(CC) -o $@ gpio.o $(LDFLAGS) $(LIBS)

.c.o:
	@echo [CC] $<
	@$(CC) -c $(CFLAGS) $< -o $@
clean:
	rm -f $(OBJ) *~ core tags spi

tags:	$(SRC)
	@echo [ctags]
	@ctags $(SRC)
depend:
	makedepend -Y $(SRC)

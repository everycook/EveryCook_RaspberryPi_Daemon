#DEBUG	= -g -O0
DEBUG	= -O3
CC	= gcc
INCLUDE	= -I/usr/local/include
CFLAGS	= $(DEBUG) -Wall $(INCLUDE) -Winline -pipe

LDFLAGS	= -L/usr/local/lib
LIBS	= -lwiringPi -lpthread

SRC 	=	firmware.c
OBJ	=	firmware.o
BINS	=	firmware

all:	firmware.o
	@echo [link]
	$(CC) -o $@ firmware.o $(LDFLAGS) $(LIBS)

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

# Makefile:
DYN_VERS_MAJ=0
DYN_VERS_MIN=2

VERSION=$(DYN_VERS_MAJ).$(DYN_VERS_MIN)
DESTDIR=/usr
PREFIX=/local

#LIBARY_NAME=libectools

#STATIC=$(LIBARY_NAME).a
#DYNAMIC=$(LIBARY_NAME).so.$(VERSION)

DAEMON=ecdaemon

#DEBUG	= -g -O0
DEBUG	= -O2
CC	= gcc
INCLUDE	= -I.
#CFLAGS	= $(DEBUG) -Wall $(INCLUDE) -Winline -pipe -fPIC
CFLAGS	= $(DEBUG) -Wall $(INCLUDE) -Winline -pipe

LDFLAGS	= -L/usr/local/lib

LIBS    = -lwiringPi -lpthread -lm #lm needed for math.h

# Should not alter anything below this line
###############################################################################

SRC	=	virtualspi.c virtuali2c.c basic_functions.c middlewareSocket.c convertFunctions.c hardwareFunctions.c daemon.c

OBJ	=	$(SRC:.c=.o)

#all:		$(STATIC) $(DYNAMIC) $(DAEMON)
all:		$(DAEMON)
#all:		$(DYNAMIC)

#$(STATIC):	$(OBJ)
#	@echo "[Link (Static)]"
#	@ar rcs $(STATIC) $(OBJ)
#	@ranlib $(STATIC)
#	# @size   $(STATIC)
#
#$(DYNAMIC):	$(OBJ)
#	@echo "[Link (Dynamic)]"
#	@$(CC) -shared -Wl,-soname,$(LIBARY_NAME).so.$(DYN_VERS_MAJ) -o $(LIBARY_NAME).so.$(VERSION) -lpthread $(OBJ)

$(DAEMON): $(OBJ)
	@echo [Link]
	@$(CC) -o $@ $(OBJ) $(LDFLAGS) $(LIBS)

virtualspi: virtualspi.o
	@echo [link]
	$(CC) -o $@ virtualspi.o $(LDFLAGS) $(LIBS)
virtuali2c: virtuali2c.o
	@echo [link]
	$(CC) -o $@ virtuali2c.o $(LDFLAGS) $(LIBS)
	
.c.o:
	@echo [Compile] $<
	@$(CC) -c $(CFLAGS) $< -o $@
	
.PHONEY:	clean
clean:
	rm -f $(OBJ) $(DAEMON)

.PHONEY:	tags
tags:	$(SRC)
	@echo [ctags]
	@ctags $(SRC)

.PHONEY:	install
install:	$(TARGET)
	@echo "[Install]"
	@install -m 0755 -d			$(DESTDIR)$(PREFIX)/include
	@install -m 0644 virtualspi.h		$(DESTDIR)$(PREFIX)/include
	@install -m 0644 virtuali2c.h		$(DESTDIR)$(PREFIX)/include
	@install -m 0644 basic_functions.h		$(DESTDIR)$(PREFIX)/include
	@install -m 0644 middlewareSocket.h		$(DESTDIR)$(PREFIX)/include
	@install -m 0644 daemon.h		$(DESTDIR)$(PREFIX)/include
	
#	@install -m 0755 -d			$(DESTDIR)$(PREFIX)/lib
#	@install -m 0755 $(LIBARY_NAME).a		$(DESTDIR)$(PREFIX)/lib
#	@install -m 0755 $(LIBARY_NAME).so.$(VERSION)		  $(DESTDIR)$(PREFIX)/lib
#	@ln -sf $(DESTDIR)$(PREFIX)/lib/$(LIBARY_NAME).so.$(VERSION) $(DESTDIR)/lib/$(LIBARY_NAME).so
#	@ln -sf $(DESTDIR)$(PREFIX)/lib/$(LIBARY_NAME).so.$(VERSION) $(DESTDIR)/lib/$(LIBARY_NAME).so.$(DYN_VERS_MAJ)
	@ldconfig
	

.PHONEY:	uninstall
uninstall:
	@echo "[UnInstall]"
	@rm -f $(DESTDIR)$(PREFIX)/include/virtualspi.h
	@rm -f $(DESTDIR)$(PREFIX)/include/virtuali2c.h
	@rm -f $(DESTDIR)$(PREFIX)/include/basic_functions.h
	@rm -f $(DESTDIR)$(PREFIX)/include/middlewareSocket.h
	@rm -f $(DESTDIR)$(PREFIX)/include/daemon.h
#	@rm -f $(DESTDIR)$(PREFIX)/lib/$(LIBARY_NAME).*
	@ldconfig


.PHONEY:	depend
depend:
	makedepend -Y $(SRC)

# DO NOT DELETE

virtualspi.o: virtualspi.h
virtuali2c.o: virtuali2c.h
basic_functions.o: basic_functions.h
middlewareSocket.o: middlewareSocket.h
convertFunctions.o: convertFunctions.h
hardwareFunctions.o: hardwareFunctions.h
daemon.o: daemon.h

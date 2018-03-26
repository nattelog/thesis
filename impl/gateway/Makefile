CC = gcc
UVDIR = libuv
CHECKDIR = check/build
TESTDIR = tests
CFLAGS =-Wall -I$(UVDIR)/include
LIBS = -luv -L$(UVDIR)/.libs
TCFLAGS =$(CFLAGS) -I$(CHECKDIR)/src -I$(CHECKDIR) -I$(TESTDIR) -I.
TLIBS = $(LIBS) -lcheck -L$(CHECKDIR)/src -lcompat -L$(CHECKDIR)/lib

DEPS = log.h state.h net.h config.h err.h machine.h
OBJ = log.o state.o net.o config.o err.o machine.o json.o
TDEPS = test.h
TOBJ = $(OBJ) test.o net_test.o
MOBJ = $(OBJ) gateway.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

%.o: $(TESTDIR)/%.c $(TESTDIR)/$(TDEPS) $(DEPS)
	$(CC) -c -o $@ $< $(TCFLAGS)

gateway: $(MOBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

test: $(TOBJ)
	$(CC) -o $@ $^ $(TCFLAGS) $(TLIBS)

install:
	./install.sh

.PHONY: clean

clean:
	rm -f *.o *~ core *~ gateway test

CC = gcc
UVDIR = libuv
CHECKDIR = check/build
TESTDIR = tests
JSONDIR = json
TPDIR = thpool
CFLAGS =-Wall -Wextra -I$(UVDIR)/include -I$(JSONDIR) -I$(TPDIR)
LIBS = -luv -L$(UVDIR)/.libs -lpthread -lm
TCFLAGS =$(CFLAGS) -I$(CHECKDIR)/src -I$(CHECKDIR) -I$(TESTDIR) -I.
TLIBS = $(LIBS) -lcheck -L$(CHECKDIR)/src -lcompat -L$(CHECKDIR)/lib

DEPS = log.h state.h net.h fs.h conf.h err.h machine.h protocol.h dispatcher.h event_handler.h
OBJ = log.o state.o net.o fs.o conf.o err.o machine.o protocol.o dispatcher.o event_handler.o $(JSONDIR)/json.o $(JSONDIR)/json-builder.o $(TPDIR)/thpool.o
TDEPS = test.h
TOBJ = $(OBJ) test.o protocol_test.o conf_test.o state_test.o event_handler_test.o
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

CC = gcc
UVDIR = libuv
CFLAGS =-Wall -I$(UVDIR)/include
LIBS = -luv -L$(UVDIR)/.libs

DEPS = log.h state.h net.h config.h err.h machine.h
OBJ = log.o gateway.o state.o net.o config.o err.o machine.o json.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

gateway: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

install:
	./install.sh

.PHONY: clean

clean:
	rm -f *.o *~ core *~

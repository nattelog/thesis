CC = gcc
UVDIR = libuv
CFLAGS =-Wall -I$(UVDIR)/include
LIBS = -luv -L$(UVDIR)/.libs

DEPS = log.h state.h
OBJ = gateway.o state.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

gateway: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

install:
	if [ ! -d "$(UVDIR)" ]; then 				\
	  git clone git@github.com:libuv/libuv.git libuv; 	\
	fi; 							\
	cd libuv;						\
	git checkout c5afc37e2a8a70d8ab0da8dac10b77ba78c0488c;	\
	./autogen.sh;						\
	./configure;						\
	make;							\
	make install;						\

clean:
	rm -f *.o *~ core *~

IDIR=../include
CC=gcc
CFLAGS=-g -Wall -I$(IDIR)
LIBS=-luv -lsqlite3 -L../lib

ODIR=obj

_DEPS =
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = impl.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

impl: $(OBJ)
	gcc -o ../$@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~

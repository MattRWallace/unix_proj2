CC          = gcc
LDFLAGS     =
CFLAGS      = -Wall -pedantic -g
DEPS        = myls.h
OBJ         = myls.o print.o util.o

EXEC        = myls


%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

../$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o

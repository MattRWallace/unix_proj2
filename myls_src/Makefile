CC          = gcc
LDFLAGS     = -lm
CFLAGS      = -Wall -pedantic -g -std=c99
DEPS        = myls.h
OBJ         = myls.o print.o util.o

EXEC        = myls


%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CLFAGS) $(LDFLAGS)

../$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

.PHONY: clean

clean:
	rm -f *.o

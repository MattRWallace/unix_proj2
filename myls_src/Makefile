CC          = gcc
LDFLAGS     = -lm
CFLAGS      = -Wall -pedantic -g
DEPS        = myls.h
OBJ         = myls.o print.o util.o
CFLAGS      = -pedantic -g -std=c99

EXEC        = myls


%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LDFLAGS)

../$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

.PHONY: clean

clean:
	rm -f *.o

CC          = gcc
LDFLAGS     = -lreadline
CFLAGS      = -pedantic -g -std=c99
DEPS        = strings.h shell.h
OBJ         = mysh.o

EXEC        = mysh

.PHONY: clean

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LDFLAGS)

../$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.o

CC          = gcc
LDFLAGS     = -lreadline
CFLAGS      = -Wall -pedantic -g
DEPS        =  
OBJ         = mysh.o

BUILDDIR    = ../

.PHONY: clean cleanall run

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LDFLAGS)

../mysh: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.o

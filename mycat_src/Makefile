CC          = gcc
LDFLAGS     = 
CFLAGS      = -Wall -pedantic -g
DEPS        =  
OBJ         = mycat.o

EXEC        = mycat

.PHONY: clean

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LDFLAGS)

../$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.o

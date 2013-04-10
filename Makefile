CC          = gcc
LDFLAGS     = -lreadline
CFLAGS      = -Wall -pedantic -g
EXEC        = mysh
DEPS        =  
OBJ         = mysh.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LDFLAGS)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.o

cleanall:
	rm -f *.o $(EXEC)

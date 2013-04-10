CC          = gcc
LDFLAGS     = 
CFLAGS      = -Wall -pedantic -g
DEPS        =  
OBJ         = mysh.o

SHELL_FOLD  = shell
CAT_FOLD    = mycat
CD_FOLD     = mycd
LS_FOLD     = myls

EXEC        = mysh

.PHONY: clean cleanall run

mysh:
	@$(MAKE) -C $(SHELL_FOLD)

clean:
	rm -f *.o

cleanall:
	rm -f *.o $(EXEC)

run:
	@. update_path.sh && mysh

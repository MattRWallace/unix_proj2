SHELL_FOLD  = mysh_src
CAT_FOLD    = mycat_src
CP_FOLD     = mycp_src
LS_FOLD     = myls_src

.PHONY: clean run mysh mycat mycp myls

all: mysh myls mycat mycd

mysh:
	$(MAKE) -C $(SHELL_FOLD)

mycat:
	$(MAKE) -C $(CAT_FOLD)

mycd:
	$(MAKE) -C $(CP_FOLD)

myls:
	$(MAKE) -C $(LS_FOLD)

clean:
	rm -f mysh mycat mycp myls

run:
	@./mysh

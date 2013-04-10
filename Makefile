SHELL_FOLD  = mysh_src
CAT_FOLD    = mycat_src
CD_FOLD     = mycd_src
LS_FOLD     = myls_src

.PHONY: clean run

mysh:
	@$(MAKE) -C $(SHELL_FOLD)

mycat:
	@$(MAKE) -C $(CAT_FOLD)

mycd:
	@$(MAKE) -C $(CD_FOLD)

myls:
	@$(MAKE) -C $(LS_FOLD)

clean:
	rm -f mysh mycat mycd myls

run:
	@. update_path.sh && mysh

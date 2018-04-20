bin_PROGRAMS += manager

manager: src/manager/manager/manager.go
	@echo 'Compiling Manager'
	$(GOC) -o $@ $<
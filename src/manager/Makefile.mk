include src/manager/fault_injector/Makefile.mk

bin_PROGRAMS += manager

manager: src/manager/manager.go
	@echo 'Compiling Manager'
	$(GOC) -o $@ $<
bin_PROGRAMS += manager fault_injector
# = src/manager/main.c

manager: src/manager/manager.go
	@echo 'Compiling Manager'
	$(GOC) -o $@ $<

fault_injector: src/manager/fault_injector.go
	@echo 'Compiling Fault Injector'
	$(GOC) -o $@ $<

#manager_SOURCES = src/manager/manager.go






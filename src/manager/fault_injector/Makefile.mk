bin_PROGRAMS += fault_injector

fault_injector: src/manager/fault_injector/fault_injector.go
	@echo 'Compiling Fault Injector'
	$(GOC) build -o $@ $<
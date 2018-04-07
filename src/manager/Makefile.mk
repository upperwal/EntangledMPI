bin_PROGRAMS += manager
# = src/manager/main.c

manager: src/manager/manager.go
	@echo 'Compiling Manager'
	$(GOC) -o $@ $<

#manager_SOURCES = src/manager/manager.go






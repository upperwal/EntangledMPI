**IMPORTANT NOTE**
This framework will only run if the program is compiled dynamically i.e. using "-dynamic" flag, with stack protection disabled i.e. using "-fno-stack-protector" flag and disabled [ASLR](https://en.wikipedia.org/wiki/Address_space_layout_randomization) (Address space layout randomization) using "sudo sysctl kernel.randomize_va_space=0" to disable ASLR only for your session.

[Stackoverflow help](https://askubuntu.com/questions/318315/how-can-i-temporarily-disable-aslr-address-space-layout-randomization)

## TODO:
1. Disable ASLR via compiler wrapper which will be created by us.


**Running mpi with gdb**
mpiexec -n 3 xterm -e gdb /home/mpiuser/entangledmpi/build_turing/reptest

## Errors:

1. *** longjmp causes uninitialized stack frame ***
	https://bugs.python.org/issue12468
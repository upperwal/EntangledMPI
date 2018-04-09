#!/bin/bash

if [ "$TRAVIS_OS_NAME" == "osx" ]; then
    cd openmpi
    if [ -f "bin/mpirun" ]; then
	echo "Using cached OpenMPI"
    else
	echo "Installing OpenMPI with homebrew"
        brew install open-mpi
	ln -s /usr/local/bin bin
	ln -s /usr/local/lib lib
	ln -s /usr/local/include include
    fi
else
    if [ -f "openmpi/bin/mpirun" ] && [ -f "openmpi-3.0.1/config.log" ]; then
	echo "Using cached OpenMPI"
	echo "Configuring OpenMPI"
	cd openmpi-3.0.1
	./configure --prefix=$TRAVIS_BUILD_DIR/openmpi CC=$C_COMPILER CXX=$CXX_COMPILER &> openmpi.configure
    else
	echo "Downloading OpenMPI Source"
	wget https://www.open-mpi.org/software/ompi/v3.0/downloads/openmpi-3.0.1.tar.gz
	tar zxf openmpi-3.0.1.tar.gz
	echo "Configuring and building OpenMPI"
	cd openmpi-3.0.1
	#./configure --prefix=$TRAVIS_BUILD_DIR/openmpi CC=$C_COMPILER CXX=$CXX_COMPILER &> openmpi.configure
	make -j4 &> openmpi.make
	make install &> openmpi.install
	cd ..
    fi
    #test -n $CC && unset CC
    #test -n $CXX && unset CXX
fi
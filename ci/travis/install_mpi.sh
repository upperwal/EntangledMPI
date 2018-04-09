#!/bin/bash

if [ "$TRAVIS_OS_NAME" == "osx" ]; then
    cd mpich
    if [ -f "bin/mpirun" ]; then
	echo "Using cached MPICH"
    else
	echo "Installing MPICH with homebrew"
        brew install mpich
	ln -s /usr/local/bin bin
	ln -s /usr/local/lib lib
	ln -s /usr/local/include include
    fi
else
    if [ -f "mpich/bin/mpirun" ] && [ -f "mpich-3.2.1/config.log" ]; then
	echo "Using cached MPICH"
	echo "Configuring MPICH"
	cd mpich-3.2.1
	./configure --prefix=$TRAVIS_BUILD_DIR/mpich &> mpich.configure
    else
	echo "Downloading MPICH Source"
	wget http://www.mpich.org/static/downloads/3.2.1/mpich-3.2.1.tar.gz
	tar zxf mpich-3.2.1.tar.gz
	echo "Configuring and building MPICH"
	cd mpich-3.2.1
	./configure --prefix=$TRAVIS_BUILD_DIR/mpich --disable-fortran &> mpich.configure
	cat mpich.configure
	make -j4 &> mpich.make
	cat mpich.make
	make install &> mpich.install
	locate mpicc
	cd ..
    fi
    #test -n $CC && unset CC
    #test -n $CXX && unset CXX
fi
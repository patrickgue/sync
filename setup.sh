#!/bin/bash
mkdir lib
cd lib

wget https://www.libssh.org/files/0.10/libssh-0.10.4.tar.xz
tar xfvz libssh-0.10.4.tar.xz

cd libssh-0.10.4
mkdir build
cd build
cmake ../
make

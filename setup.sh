#!/bin/bash
rm -rf lib
mkdir -p lib/opt
cd lib

#wget https://gnupg.org/ftp/gcrypt/libgcrypt/libgcrypt-1.10.1.tar.bz2
#tar xfvz libgcrypt-1.10.1.tar.bz2

#mkdir libgcrypt-build
#cd libgcrypt-build
#../libgcrypt-1.10.1/configure --prefix=$PWD/../opt/
#make
#make install

wget https://www.openssl.org/source/openssl-3.1.0.tar.gz
tar xfvz openssl-3.1.0.tar.gz
mkdir openssl-build
cd openssl-build
../openssl-3.1.0/Configure --prefix=$PWD/../opt
make -j 4
make install

cd ../


wget https://www.libssh2.org/download/libssh2-1.10.0.tar.gz
tar xfvz libssh2-1.10.0.tar.gz

mkdir libssh2-build
cd libssh2-build
../libssh2-1.10.0/configure --with-crypto=openssl --prefix=$PWD/../opt/
make -j 4
make install

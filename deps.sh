#!/usr/bin/env bash

cd "$(dirname "$0")"
JOBS=8
OS=$1
case "$OS" in
mac)
    ;;
ubuntu)
    ;;
centos)
    ;;
none)
    ;;
*)
    echo "Usage: $0 [ubuntu|mac|centos|none]"
    exit 1
esac

if [ $OS = mac ]; then
    echo "Installing dependencies for MacOS..."
    brew install openssl git gnu-getopt gflags protobuf leveldb cmake flex bison
elif [ $OS = ubuntu ]; then
    echo "Installing dependencies for Ubuntu..."
    sudo apt-get install git g++ make libssl-dev
    sudo apt-get install coreutils libgflags-dev libprotobuf-dev libprotoc-dev protobuf-compiler libleveldb-dev libsnappy-dev
    sudo apt-get install flex bison
elif [ $OS = centos ]; then
    echo "Installing dependencies for CentOS..."
    sudo yum install epel-release
    sudo yum install git gcc-c++ make openssl-devel
    sudo yum install gflags-devel protobuf-devel protobuf-compiler leveldb-devel
    sudo yum install flex bison
else
    echo "Skipping dependencies installation..."
fi


cd third_party
if [ ! -e brpc ]; then
    echo "Cloning brpc..."
    git clone https://github.com/brpc/brpc.git
fi
cd brpc
git checkout master
echo "Updating brpc..."
git pull

#build brpc
echo "Building brpc..."
mkdir -p _build
cd _build
cmake ..
make -j$JOBS

cd ../../../

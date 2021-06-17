#!/usr/bin/env bash
set -x

cd "$(dirname "$0")"
JOBS=8
OS=$1
case "$OS" in
ubuntu)
    ;;
centos)
    ;;
none)
    ;;
*)
    echo "Usage: $0 [ubuntu|centos|none]"
    exit 1
esac

# Install dependencies
if [ $OS = ubuntu ]; then
    echo "Installing dependencies for Ubuntu..."
    apt-get update && apt-get install -y --no-install-recommends sudo cmake wget vim curl ca-certificates
    update-ca-certificates
    sudo apt-get install -y git g++ make libssl-dev libcurl4-openssl-dev
    sudo apt-get install -y coreutils libgflags-dev libprotobuf-dev libprotoc-dev protobuf-compiler libleveldb-dev libsnappy-dev
    sudo apt-get install -y flex bison
elif [ $OS = centos ]; then
    echo "Installing dependencies for CentOS..."
    yum install -y sudo cmake wget vim curl ca-certificates
    update-ca-trust
    sudo yum install -y epel-release centos-release-scl
    sudo yum install -y devtoolset-7-gcc-c++ && source /opt/rh/devtoolset-7/enable
    sudo yum install -y git make openssl-devel libcurl-devel
    sudo yum install -y gflags-devel protobuf-devel protobuf-compiler leveldb-devel
    sudo yum install -y flex bison
else
    echo "Skipping dependencies installation..."
fi

# Setup git SSL
git config --global http.sslVerify false

# Build boost
cd third_party
if [ ! -e boost ]; then
    echo "Cloning boost..."
    git clone --recursive https://github.com/boostorg/boost.git
fi
echo "Updating boost..."
cd boost
git checkout master
git pull
git checkout boost-1.76.0

echo "Building boost..."
mkdir -p _build/output
./bootstrap.sh --prefix=./_build/output
./b2 install
cd ..

# Build brpc
if [ ! -e brpc ]; then
    echo "Cloning brpc..."
    git clone https://github.com/apache/incubator-brpc.git brpc
fi
echo "Updating brpc..."
cd brpc
git checkout master
git pull
git checkout 0.9.7

echo "Building brpc..."
mkdir -p _build
cd _build
cmake ..
make -j$JOBS

cd ../../../

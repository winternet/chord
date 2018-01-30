# chord
[![Build Status](https://circleci.com/gh/winternet/chord/tree/master.svg?style=shield&circle-token=06884550effac32786aa01b3638bdd15e8baa03b)](https://circleci.com/gh/winternet/chord) [![codecov](https://codecov.io/gh/winternet/chord/branch/master/graph/badge.svg)](https://codecov.io/gh/winternet/chord)

## pre-requisites

* common
```sh
 $ [sudo] apt-get -y install build-essential autoconf libtool git curl cmake unzip software-properties-common wget
```

* boost libraries
```sh
 $ [sudo] apt-get install libboost-dev
```

* leveldb libraries
```sh
 $ [sudo] apt-get -y install libleveldb-dev
```

* yaml-cpp libraries
```sh
 $ [sudo] apt-get -y install libyaml-cpp-dev
```

* xattr libraries
```sh
 $ [sudo] apt-get -y install attr-dev
```

* openssl libraries
```sh
 $ [sudo] apt-get -y install openssl libssl-dev
```

* grpc
```sh
 $ git clone -b $(curl -L https://grpc.io/release) https://github.com/grpc/grpc
 $ cd grpc
 $ git submodule update --init
 $ make -j4 CFLAGS='-Wno-implicit-fallthrough' CXXFLAGS='-Wno-implicit-fallthrough'
 $ sudo make install
 $ ldconfig
```

* protobuf
```sh
 $ git clone -b v3.0.2 https://github.com/google/protobuf.git protobuf
 $ cd protobuf
 $ ./autogen.sh && ./configure
 $ make -j4 && make install
 $ ldconfig
```

## installation

```sh
 $ mkdir build
 $ cd build
 $ CXX=g++-7 CC=gcc-7 cmake ..
 $ cmake --build . -- -j4 
```


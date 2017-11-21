[![Build Status](https://circleci.com/gh/winternet/chord/tree/master.svg?style=shield&circle-token=06884550effac32786aa01b3638bdd15e8baa03b)](https://circleci.com/gh/winternet/chord)

# chord

## pre-requisites

* common
```sh
 $ [sudo] apt-get install build-essential autoconf libtool git curl
```

* boost libraries (>= 1.65)
```sh
 $ [sudo] apt-get install libboost-dev
```

* grpc / protobuf
```sh
 $ git clone -b $(curl -L https://grpc.io/release) https://github.com/grpc/grpc
 $ cd grpc
 $ git submodule update --init
 $ make CFLAGS='-Wno-implicit-fallthrough' CXXFLAGS='-Wno-implicit-fallthrough'
 $ sudo make install
```

```sh
 $ cd third_party/protobuf
 $ ./autogen.sh
 $ ./configure && make
 $ sudo make install
```

# chord
[![Build Status](https://circleci.com/gh/winternet/chord/tree/master.svg?style=shield&circle-token=06884550effac32786aa01b3638bdd15e8baa03b)](https://circleci.com/gh/winternet/chord) [![codecov](https://codecov.io/gh/winternet/chord/branch/master/graph/badge.svg)](https://codecov.io/gh/winternet/chord)

## Using the docker image

The easiest way to get started with chord is using a small docker image (~12 MB) provided by the repository `winternet1337/chord`. The images are automatically built and pushed to docker hub after each commit - provided all tests passed successfully.

### First steps

To run a node in interactive mode and cleanup automatically afterwards issue `$ docker run -ti --rm winternet1337/chord`. This command will bootstrap the container with random node uuid on default port `50050`. 
To stop the container issue `Ctrl+C`.

In order to print chord's help just append the `--help` argument (`-h` for short): 
```sh
 $ docker run -ti --rm winternet1337/chord --help
 [program options]:
  -h [ --help ]           produce help message
  -c [ --config ] arg     path to the yaml configuration file.
  -j [ --join ] arg       join to an existing address.
  -b [ --bootstrap ]      bootstrap peer to create a new chord ring.
  -n [ --no-controller ]  do not start the controller.
  -u [ --uuid ] arg       client uuid.
  --bind arg              bind address that is promoted to clients.
```

To configure our node we could pass some of the arguments to the container, however, it is far more convenient and powerful to use a configuration file. For this to work, we create one on our docker host machine and mount the volume within the docker container.

## Installing chord manually

If you prefer a manual installation, have a look at the following instructions which are mainly copied from the [cirecleci configuration](https://github.com/winternet/chord/blob/master/.circleci/config.yml).

### Installing all dependencies

* Install common dependencies
```sh
$ apt-get update
$ apt-get -y install build-essential \
            git \
            cmake \
            autoconf \
            libtool \
            libboost-system-dev \
            libboost-serialization-dev \
            libleveldb-dev \
            libyaml-cpp-dev \
            libgrpc++-dev \
            libboost-thread-dev \
            libboost-program-options-dev \
            libssl-dev
```

* Install GRPC
```sh
 $ git clone -b v1.15.1 https://github.com/grpc/grpc /opt/grpc && cd /opt/grpc
 $ git submodule update --init
 $ make -j4 CFLAGS='-Wno-error -Wno-expansion-to-defined' && make install CFLAGS='-Wno-error -Wno-expansion-to-defined'
 $ ldconfig
```

* Install Protobuf
```sh
 $ git clone -b v3.6.1 https://github.com/google/protobuf.git /opt/protobuf && cd /opt/protobuf
 $ ./autogen.sh
 $ ./configure
 $ make -j4 && make install
 $ ldconfig
 $ protoc --version
```

### Install chord

```sh
 $ git clone https://github.com/winternet/chord.git /opt/chord && cd /opt/chord
 $ mkdir build && cd build
 $ cmake ..
 $ cmake --build . -- -j4 
```


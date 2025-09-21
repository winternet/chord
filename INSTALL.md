# chord
[![Build Status](https://circleci.com/gh/winternet/chord/tree/main.svg?style=shield&circle-token=06884550effac32786aa01b3638bdd15e8baa03b)](https://circleci.com/gh/winternet/chord) [![codecov](https://codecov.io/gh/winternet/chord/branch/main/graph/badge.svg)](https://codecov.io/gh/winternet/chord)

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

If you prefer a manual installation, have a look at the following instructions which are mainly copied from the [cirecleci configuration](https://github.com/winternet/chord/blob/main/.circleci/config.yml).

### Configure conan2

The following section explains how to configure conan2 to be able to compile the sources and its dependencies.

#### Missing uint64_t errors

Some of the dependencies are not ready for modern `gcc/libstdc++` because of missing `cstdint` includes. To fix this add the following tool configuration to your conan profile, e.g. `~/.conan2/profiles/default`:
```
[conf]
tools.build:cxxflags=["-include", "cstdint"]
```

#### Boost failing to compile due to PCH: No such file or directory

If you encounter the follwing issues during compilation of `boost`,

```<command-line>: fatal error: ~/.conan2/p/b/boostcd0368e70ce3f/b/build-debug/boost/bin.v2/libs/math/build/gcc-15/dbg/x86_6/cxstd-2b-iso/lnk-sttc/nm-on/thrd-mlt/vsblt-hdn/pch: No such file or directory compilation terminated.

...failed updating 0 target...
```

disable using pre-compiled headers (PCH) in the conan profile, e.g. `~/.conan2/profiles/default`:
```
[conf]
tools.build:cxxflags=["-DBOOST_NO_PCH"]
```

### Installing all dependencies

* Install common dependencies
```sh
$ apt-get update
$ apt-get -y install build-essential \
            git \
            cmake \
            autoconf \
            python3\
            python3-pip
$ pip3 install conan && source ~/.profile
$ conan profile detect -e
```

### Install chord

```sh
 $ git clone https://github.com/winternet/chord.git /opt/chord && cd /opt/chord
 $ conan install . --build=missing --output-folder=build --settings=build_type=Debug 
 $ cd build && cmake ..
 $ cmake --build . -- -j4 
```

#### Release version

```sh
$ git clone https://github.com/winternet/chord.git /opt/chord && cd /opt/chord
$ conan install . --build=missing --output-folder=build --settings=build_type=Release
$ cd build && cmake .. -DCMAKE_BUILD_TYPE=Release
$ cmake --build . -- -j4 
```

#### Debug version

https://github.com/conan-io/conan/issues/13478#issuecomment-1475389368

```sh
$ git clone https://github.com/winternet/chord.git /opt/chord && cd /opt/chord
$ conan install . --build=missing --output-folder=build -s build_type=Debug -s "&:build_type=Debug
$ cd build
$ cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug
$ cmake --build . -- -j4 
```

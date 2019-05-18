# chord
[![Build Status](https://circleci.com/gh/winternet/chord/tree/master.svg?style=shield&circle-token=06884550effac32786aa01b3638bdd15e8baa03b)](https://circleci.com/gh/winternet/chord) [![codecov](https://codecov.io/gh/winternet/chord/branch/master/graph/badge.svg)](https://codecov.io/gh/winternet/chord)

## Using the docker image

The easiest way to get started with chord is using a small docker image (~12 MB) provided by the repository `winternet1337/chord`. The images are automatically built and pushed to docker hub after each commit - provided all tests passed successfully.

> **INFO**: There are currently issues concerning grpc on libmusl which cause a) the application to sigsegv because too small default stack size and b) connectivity issues within the grpc streaming api.

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

### The node configuration

Sample configurations can be found in the sourcecode repository under the [config](https://github.com/winternet/chord/tree/master/config)-folder. Either use this samples or copy & paste the one below to e.g. /tmp/chord/config/node0.yml:

```yaml
version: 1

## data
data-directory: "/data"
meta-directory: "/meta"

## networking
bind-addr: "0.0.0.0:50050"
join-addr: "0.0.0.0:50051"
bootstrap: Yes
no-controller: No

## details
stabilize_ms: 10000
check_ms: 10000

## replication / striping
replication-count: 1

uuid: "0"

## logging
logging:
  level: trace
  sinks:
    CONSOLE_SINK:
      type: "console"
    FILE_SINK:
      type: "file-rotating"
      path: "/logs/chord0.log"
  loggers:
    CHORD_LOG:
      sinks: [FILE_SINK]
      filter: "^chord[.](?!fs)"
    CHORD_FS_LOG:
      sinks: [CONSOLE_SINK]
      filter: "^chord[.]fs"
      level: trace
```

The configuration should be quite self-explanatory. A more detailed description of the configuration will be provided in the wiki.

### Starting configured node

To start the node with the yaml configuration file, we need to mount it to the container. 

```sh
 $ docker run -ti --rm \
        -v /tmp/chord/config:/etc/chord \
        winternet1337/chord -c /etc/chord/node0.yml
[<timestamp>] [chord.fs.metadata.manager] [trace] [ADD] chord:///
[<timestamp>] [chord.fs.metadata.manager] [trace] [ADD] .
```

Since we restricted the console logging to the filesystem part, we are greeted by the metadata manager creating the root of our p2p filesystem - waiting for something to happen.

### Setup chord cluster

The next section describes how to setup a small local cluster consisting of two nodes. The section closes with storing a folder within our cluster.

#### Forwarding ports and bind address

To wire our different nodes locally we will exploit docker's `--net=host` option. Note that this is not the preferred way but its ok for showing the basic concepts. We start by mounting more volumes so that all (meta-)data is written to the docker host filesystem.

```sh
 $ docker run -ti --rm --net=host \
        -v /tmp/chord/config:/etc/chord \
        -v /tmp/chord/data0:/data \
        -v /tmp/chord/meta0:/meta \
        winternet1337/chord -c /etc/chord/node0.yml
```

Copy the `/tmp/chord/config/node0.yml` to `/tmp/chord/config/node1.yml` and change the uuid to a value near `(2^256)/2` so that all files are distributed equally. Also change the bind address to `bind-addr: "0.0.0.0:50051"` and the join address to `join-addr: "0.0.0.0:50050"`. 

On a different shell start another docker instance with our second configuration and different (meta-)data directories.

```sh
 $ docker run -ti --rm --net=host \
        -v /tmp/chord/config:/etc/chord \
        -v /tmp/chord/data1:/data \
        -v /tmp/chord/meta1:/meta \
        winternet1337/chord -c /etc/chord/node1.yml
```


TODO
[ ] port forwarding and promote the correct bind address (simple host network).
[ ] volume mounts for data, metadata and log files.
[ ] connect and put a file using a client.


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


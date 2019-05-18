```
                                                      
     _/_/_/  _/                                  _/   
  _/        _/_/_/      _/_/    _/  _/_/    _/_/_/    
 _/        _/    _/  _/    _/  _/_/      _/    _/     
_/        _/    _/  _/    _/  _/        _/    _/      
 _/_/_/  _/    _/    _/_/    _/          _/_/_/       
                                                      
                                                      
```

[![Build Status](https://circleci.com/gh/winternet/chord/tree/master.svg?style=shield&circle-token=06884550effac32786aa01b3638bdd15e8baa03b)](https://circleci.com/gh/winternet/chord) [![codecov](https://codecov.io/gh/winternet/chord/branch/master/graph/badge.svg)](https://codecov.io/gh/winternet/chord)

## Installation

See [INSTALL.md](INSTALL.md) for installation instructions.

## Development

### musl-libc functional differences

According to the [functional differences from glibc](https://wiki.musl-libc.org/functional-differences-from-glibc.html#Thread-stack-size) the default stack size is ~80k. This will cause a segfault within `chord.fs.client.cc` during allocating the buffer (`512*1024` bytes).

### GDB within docker container

To enable debugging within docker containers, the container has to be started with the following options

```sh
$ docker run --rm -it --cap-add=SYS_PTRACE --security-opt seccomp=unconfined <DOCKER-IMAGE-NAME>
```

Docker Images
=============

Package Builds
--------------

For installation checkout INSTALL.md of this project. After the binaries have been built use cpack-tool to build the packages.

### RPM

To build RPMs:

```sh
docker pull dokken/centos-stream-8
dnf -y install git \
            cmake \
            rpm-build \
            python3 \
            gcc-c++
pip3 install conan
git clone https://github.com/winternet/chord.git && cd chord
mkdir build && cd build
conan config set general.revisions_enabled=True
conan remote add winternet-conan-virt https://winternet.jfrog.io/artifactory/api/conan/conan-virt
cpack -G RPM
```

### DEB

´´´sh
docker pull ubuntu:20.04
$ apt-get update
$ apt-get -y install build-essential \
            git \
            cmake \
            autoconf \
            python3\
            python3-pip
cpack -G DEB
´´´

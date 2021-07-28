FROM ubuntu:18.04 AS build

#
# install dependencies
#
RUN apt-get update \
    && apt-get -y install build-essential \
            git \
            gdb \
            cmake \
            autoconf \
            libtool \
            libboost-system-dev \
            libboost-serialization-dev \
            libleveldb-dev \
            librocksdb-dev \
            libyaml-cpp-dev \
            libgrpc++-dev \
            libboost-thread-dev \
            libboost-program-options-dev \
            libssl-dev

#
# install grpc
#
RUN git clone -b v1.20.1 https://github.com/grpc/grpc /opt/grpc && cd /opt/grpc \
    && git submodule update --init \
    && make -j$(nproc) CFLAGS='-Wno-error -Wno-expansion-to-defined' && make install CFLAGS='-Wno-error -Wno-expansion-to-defined'

#
# install protobuf
#
RUN git clone -b v3.6.1 https://github.com/google/protobuf.git /opt/protobuf && cd /opt/protobuf/cmake \
    && mkdir build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
                -Dprotobuf_BUILD_EXAMPLES=OFF \
                -Dprotobuf_BUILD_SHARED_LIBS=ON \
                -Dprotobuf_BUILD_TESTS=OFF \
                -Dprotobuf_MSVC_STATIC_RUNTIME=OFF \
                -Dprotobuf_WITH_ZLIB=ON \
                -Dprotobuf_BUILD_PROTOC_BINARIES=ON \
    && make -j$(nproc) && make install \
    && ldconfig

#
# build project
#
RUN git clone https://github.com/winternet/chord.git /opt/chord \
    && cd /opt/chord && mkdir build && cd build \
    && cmake .. \
        -DBUILD_GMOCK=OFF \
        -DBUILD_TESTING=OFF \
        -DINSTALL_GTEST=OFF \
        -Dchord_BUILD_TESTS=OFF \
        -DCMAKE_BUILD_TYPE=Release \
    && cmake --build . -- -j$(nproc)

#
# create chroot env
#
RUN mkdir -p chroot/bin \
    && cp ./chord ./chroot/bin \
    && ldd ./chord | grep -oP '/[^ ]*' | xargs -I '{}' cp --parents -v '{}' ./chroot/

#
# node image
#
FROM scratch

COPY --from=build /opt/chord/build/chroot /

EXPOSE 50050

ENTRYPOINT ["/bin/chord"]
CMD ["-b"]

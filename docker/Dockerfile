FROM alpine:latest AS base

# MAINTAINER Foo fo@bar.org
# ARG VERSION=0.0.0

#
# basic shared dependencies
#
RUN apk update \
    && apk add --no-cache \
                     boost-system \
                     boost-program_options \
                     boost-serialization \
                     yaml-cpp \
                     -X http://dl-cdn.alpinelinux.org/alpine/edge/community leveldb \
                     -X http://dl-cdn.alpinelinux.org/alpine/edge/testing grpc

#
# the build image
#
FROM base AS build

#
# add build dependencies
#
RUN apk add --no-cache \
          alpine-sdk \
          cmake \
          boost-dev \
          -X http://dl-cdn.alpinelinux.org/alpine/edge/testing grpc-dev \
          -X http://dl-cdn.alpinelinux.org/alpine/edge/testing protobuf-dev \
          -X http://dl-cdn.alpinelinux.org/alpine/edge/community leveldb-dev \
          -X http://dl-cdn.alpinelinux.org/alpine/edge/community yaml-cpp-dev \
    && git clone https://github.com/winternet/chord.git /opt/chord

#
# actual build
#
WORKDIR /opt/chord
RUN mkdir build \
    && cd build \
    && cmake .. \
        -DBUILD_GMOCK=OFF \
        -DBUILD_TESTING=OFF \
        -DINSTALL_GTEST=OFF \
        -Dchord_BUILD_TESTS=OFF \
        -DCMAKE_BUILD_TYPE=Release \
    && make -j4 \
    && make install

#
# node image
#
FROM base
COPY --from=build /opt/chord/build/chord /usr/local/bin/chord
COPY --from=build /opt/chord/build/libchord++.so /usr/local/lib/libchord++.so

EXPOSE 50050

ENTRYPOINT ["chord"]
CMD ["-b"]

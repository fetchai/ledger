FROM centos/devtoolset-4-toolchain-centos7:latest

ENV CC=/usr/bin/clang
ENV CXX=/usr/bin/clang++

USER root

RUN groupadd sudo && \
    usermod -a -G sudo default && \
    echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

RUN yum install -y sudo && \
    yum install -y make && \
    yum install -y cmake && \
    yum install -y libpng-devel && \
    yum install -y zlib-devel && \
    yum install -y openssl-devel && \
    yum install -y python-devel && \
    yum install -y clang

USER default

WORKDIR /build
ENTRYPOINT []


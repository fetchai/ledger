FROM centos/devtoolset-4-toolchain-centos7:latest

ENV CC=/usr/bin/clang
ENV CXX=/usr/bin/clang++

USER root

RUN groupadd sudo && \
    usermod -a -G sudo default && \
    echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

RUN yum update -y && \
    yum install -y sudo && \
    yum install -y make && \
    yum install -y cmake && \
    yum install -y libpng-devel && \
    yum install -y zlib-devel && \
    yum install -y openssl-devel && \
    yum install -y python-devel && \
    yum install -y clang && \
    yum install -y epel-release && \
    yum install -y python-pip && \
    pip install --upgrade pip && \
    pip install --upgrade cldoc &&  \
    yum install -y git

USER default

# vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
# ==== BOOST(1.67.0) & GOURCE ====
# This setup is here to build the `Gource`, and SHOULD be relocated to dedicated
# dockerfile & container.
USER root

RUN yum install -y SDL2-devel && \
    yum install -y SDL2_image-devel && \
    yum install -y pcre-devel && \
    yum install -y freetype-devel && \
    yum install -y glew-devel && \
    yum install -y glm-devel && \
    yum install -y tinyxml-devel && \
    yum install -y autoconf && \
    yum install -y automake && \
    yum install -y wget && \
    yum install -y which
#    yum install -y boost-devel

RUN mkdir /src && \
    chown default /src && \
    chgrp default /src

USER default

# Starting C++ build in new *login* shell in order to get correct enviroment setup (env. variables) for C++ build. This is necessary since the straight shell from `RUN` instruction is not login shell, and so its environment does not match the full blown propper environment setup with all the stuff for C++ build user normally gets after normal login (e.g. ./configure and make would fail with obscure errors).
RUN bash --login -c "\
    cd /src && \
    wget http://sourceforge.net/projects/boost/files/boost/1.67.0/boost_1_67_0.tar.gz && \
    tar -xzvf boost_1_67_0.tar.gz && \
    cd boost_1_67_0 && \
    ./bootstrap.sh --with-toolset=clang --prefix=/usr/local && \
    NUMBER_OF_CPU_CORES=`getconf _NPROCESSORS_ONLN` && \
    echo NUMBER_OF_CPU_CORES: \$NUMBER_OF_CPU_CORES && \
    sudo ./b2 -j\$NUMBER_OF_CPU_CORES install --with=all"

RUN bash --login -c "\
    cd /src && \
    git clone https://github.com/acaudwell/Gource.git && \
    cd Gource && \
    git pull && \
    git submodule sync --recursive && \
    git submodule update --init --recursive && \
    ./autogen.sh && \
    ./configure && \
    make -j && \
    sudo make -j install"
# ==== BOOST(1.67.0) & GOURCE ====
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

WORKDIR /build
ENTRYPOINT []


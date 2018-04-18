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
# ==== GOURCE ====
# This setup is here to build the `Gource`, and SHOULD be relocated to dedicated
# dockerfile & container.
USER root
RUN yum install -y SDL2-devel && \
    yum install -y SDL2_image-devel && \
    yum install -y pcre-devel && \
    yum install -y freetype-devel && \
    yum install -y glew-devel && \
    yum install -y glm-devel && \
    yum install -y boost-devel && \
    yum install -y tinyxml-devel && \
    yum install -y autoconf && \
    yum install -y automake

RUN mkdir /src && \
    chown default /src && \
    chgrp default /src

USER default

# Starting C++ build in new login shell in order to read & set correct enviroment (env. variables) for C++ bils. It is necessary since the shell from `RUN` command does not have all env. variables set for C++ build (e.g. ./configure and make would fail with obscure error)
RUN bash --login -c "cd /src && \
    git clone https://github.com/acaudwell/Gource.git && \
    cd Gource && \
    ./autogen.sh && \
    ./configure && \
    make -j && \
    sudo make -j install"
# ==== GOURCE ====
# ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

WORKDIR /build
ENTRYPOINT []


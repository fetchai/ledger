FROM ubuntu:18.04

RUN apt-get update &&                                                        \
    apt-get upgrade -y &&                                                    \
    apt-get install -y                                                       \
       clang                                                                 \
       clang-7                                                               \
       gcc-7                                                                 \
       g++-7                                                                 \
       gcc-8                                                                 \
       g++-8                                                                 \
       gdb                                                                   \
       clang-tidy                                                            \
       clang-format-6.0                                                      \
       cmake-curses-gui                                                      \
       llvm                                                                  \
       sudo                                                                  \
       vim                                                                   \
       make                                                                  \
       cmake                                                                 \
       git                                                                   \
       libboost-all-dev=1.65.1.0ubuntu1                                      \
       less                                                                  \
       curl                                                                  \
       wget                                                                  \
       zlib1g-dev                                                            \
       python3-dev                                                           \
       python3-pip

# This adds the 'default' user to sudoers with full privileges:
RUN HOME=/home/default &&                                                    \
    mkdir -p ${HOME} &&                                                      \
    GROUP_ID=1000 &&                                                         \
    USER_ID=1000 &&                                                          \
    groupadd -r default -f -g "$GROUP_ID" &&                                 \
    useradd -u "$USER_ID" -r -g default -d "$HOME" -s /sbin/nologin          \
    -c "Default Application User" default &&                                 \
    chown -R "$USER_ID:$GROUP_ID" ${HOME} &&                                 \
    usermod -a -G sudo default &&                                            \
    echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

ARG OPENSSL_VERSION_GIT_SELECTOR="OpenSSL_1_1_1b"

ENV OPENSSL_VERSION_GIT_SELECTOR="${OPENSSL_VERSION_GIT_SELECTOR}"
ENV CC=/usr/bin/clang
ENV CXX=/usr/bin/clang++
ENV CCACHE_DIR=/build/docker_ccache

RUN OPENSSL_SRC_DIR="/home/default/openssl" &&                               \
    ORIG_DIR="$(pwd)" &&                                                     \
    mkdir -p "${OPENSSL_SRC_DIR}" &&                                         \
    git clone -b "${OPENSSL_VERSION_GIT_SELECTOR}"                           \
        --single-branch --depth 1                                            \
        https://github.com/openssl/openssl.git                               \
        "${OPENSSL_SRC_DIR}" &&                                              \
    cd "${OPENSSL_SRC_DIR}" &&                                               \
    git submodule sync --recursive &&                                        \
    git submodule update --init --recursive &&                               \
    ./config &&                                                              \
    make -j &&                                                               \
    make test &&                                                             \
    make install &&                                                          \
    cd "${ORIG_DIR}" &&                                                      \
    rm -rf "${OPENSSL_SRC_DIR}"

# Required for integration tests/style checks
RUN pip3 install PyYAML &&                                                   \
    pip3 install -U fetchai-ledger-api &&                                    \
    pip3 install -i https://test.pypi.org/simple/                            \
        fetchai-netutils==0.0.1 &&                                           \
    pip3 install autopep8==1.4.4 cmake_format==0.5.1

ENV LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib:/usr/lib

USER default

WORKDIR /build
ENTRYPOINT []

#FROM registry.access.redhat.com/rhscl/devtoolset-7-toolchain-rhel7
#FROM rhscl/devtoolset-4-toolchain-rhel7
FROM centos/devtoolset-4-toolchain-centos7:latest
USER 0
#RUN cat /etc/yum/pluginconf.d/subscription-manager.conf
#COPY subscription-manager.conf /etc/yum/pluginconf.d/subscription-manager.conf
#RUN cat /etc/yum/pluginconf.d/subscription-manager.conf
RUN yum install --assumeyes sudo cmake libpng-dev zlib1g-dev libssl-dev
USER 1001

#FROM madduci/docker-ubuntu-cpp:clang-3.9
#RUN apt-get update && \
#    apt-get upgrade --fix-missing --fix-broken --assume-yes && \
#    apt-get install --fix-missing --fix-broken --assume-yes \
#        libpng-dev zlib1g-dev libssl-dev

#RUN apt-get update
#RUN apt-get upgrade --fix-missing --fix-broken --assume-yes
#RUN apt-get install --fix-missing --fix-broken --assume-yes libpng-dev zlib1g-dev libssl-dev

WORKDIR /build
ENTRYPOINT []


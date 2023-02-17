ARG base_image=ubuntu
ARG base_tag=22.04

# Base image with environment variables set
FROM ${base_image}:${base_tag} AS base

# Set bash as the default shell
SHELL ["/bin/bash", "-c"]

ARG psyllid_tag=beta
ARG psyllid_subdir=psyllid
ARG build_type=Release

ENV PSYLLID_BUILD_TYPE=$build_type

ENV P8_ROOT=/usr/local/p8
ENV PSYLLID_TAG=${psyllid_tag}
ENV PSYLLID_INSTALL_PREFIX=${P8_ROOT}/${psyllid_subdir}/${PSYLLID_TAG}

ENV PATH="${PATH}:${PSYLLID_INSTALL_PREFIX}"

# Build image with dev dependencies
FROM base AS build

RUN apt-get update &&\
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
        build-essential \
        cmake \
        git \
        openssl \
        libfftw3-dev \
        libboost-chrono-dev \
        libboost-filesystem-dev \
        libboost-system-dev \
        libhdf5-dev \
        librabbitmq-dev \
        libyaml-cpp-dev \
        rapidjson-dev \
        &&\
    apt-get clean &&\
    rm -rf /var/lib/apt/lists/* &&\
    /bin/true

COPY external /tmp_source/external
COPY monarch /tmp_source/monarch
COPY sandfly /tmp_source/sandfly
COPY source /tmp_source/source
COPY CMakeLists.txt /tmp_source/CMakeLists.txt
COPY .git /tmp_source/.git
COPY .gitmodules /tmp_source/.gitmodules
COPY PsyllidConfig.cmake.in /tmp_source/PsyllidConfig.cmake.in

## store cmake args because we'll need to run twice (known package_builder issue)
## use `extra_cmake_args` to add or replace options at build time; CMAKE_CONFIG_ARGS_LIST are defaults
ARG extra_cmake_args=""
ENV CMAKE_CONFIG_ARGS_LIST="\
      -D CMAKE_BUILD_TYPE=$PSYLLID_BUILD_TYPE \
      -D CMAKE_INSTALL_PREFIX:PATH=$PSYLLID_BUILD_PREFIX \
      -D Psyllid_ENABLE_FPA=FALSE \
      ${extra_cmake_args} \
      ${OS_CMAKE_ARGS} \
      "

RUN mkdir -p /tmp_source/build &&\
    cd /tmp_source/build &&\
    cmake ${CMAKE_CONFIG_ARGS_LIST} .. &&\
    cmake ${CMAKE_CONFIG_ARGS_LIST} .. &&\
    make install &&\
    /bin/true

# Final production image
FROM base

RUN apt-get update &&\
    DEBIAN_FRONTEND=noninteractive apt-get install -y \
        build-essential \
        libssl3 \
        libfftw3-double3 \
        libboost-chrono1.74.0 \
        libboost-filesystem1.74.0 \
        libboost-system1.74.0 \
        libhdf5-cpp-103 \
        librabbitmq4 \
        libyaml-cpp0.7 \
        rapidjson-dev \
        &&\
    apt-get clean &&\
    rm -rf /var/lib/apt/lists/* &&\
    /bin/true

# for now we must grab the extra dependency content as well as psyllid itself
COPY --from=build $PSYLLID_BUILD_PREFIX $PSYLLID_BUILD_PREFIX

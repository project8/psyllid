ARG base_image=ubuntu
ARG base_tag=22.04

# Base image with environment variables set
FROM ${base_image}:${base_tag} AS base

# Set bash as the default shell
SHELL ["/bin/bash", "-c"]

ARG psyllid_tag=beta
ARG psyllid_subdir=psyllid
ARG build_type=Release
ARG narg=2


ENV P8_ROOT=/usr/local/p8
ENV PSYLLID_TAG=${psyllid_tag}
ENV PSYLLID_INSTALL_PREFIX=${P8_ROOT}/${psyllid_subdir}/${PSYLLID_TAG}
ENV NARG=${narg}

ENV PATH="${PATH}:${PSYLLID_INSTALL_PREFIX}"

# Build image with dev dependencies
FROM base AS deps

# use quill_checkout to specify a tag or branch name to checkout
ARG quill_checkout=v8.1.1
ENV QUILL_CHECKOUT=${quill_checkout}

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
    cd /usr/local &&\
    git clone https://github.com/odygrd/quill.git &&\
    cd quill &&\
    git checkout ${QUILL_CHECKOUT} &&\
    mkdir build &&\
    cd build &&\
    cmake .. &&\
    make -j${narg} install &&\
    cd / &&\
    rm -rf /usr/local/quill &&\
    /bin/true

# Build psyllid in the deps image
FROM deps AS build

COPY . /tmp_source

## store cmake args because we'll need to run twice (known package_builder issue)
## use `extra_cmake_args` to add or replace options at build time; CMAKE_CONFIG_ARGS_LIST are defaults
ARG extra_cmake_args=""
ENV CMAKE_CONFIG_ARGS_LIST="\
      -D CMAKE_BUILD_TYPE=$build_type \
      -D CMAKE_INSTALL_PREFIX:PATH=$PSYLLID_INSTALL_PREFIX \
      -D Psyllid_ENABLE_FPA=FALSE \
      ${extra_cmake_args} \
      "

RUN mkdir -p /tmp_source/build &&\
    cd /tmp_source/build &&\
    cmake ${CMAKE_CONFIG_ARGS_LIST} .. &&\
    cmake ${CMAKE_CONFIG_ARGS_LIST} .. &&\
    make -j${NARG} install &&\
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
COPY --from=build /usr/local/lib /usr/local/lib
COPY --from=build $PSYLLID_INSTALL_PREFIX $PSYLLID_INSTALL_PREFIX

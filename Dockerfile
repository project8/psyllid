## Note that these IMG_* ARG values are defaults, but actual automated builds use
## values which stored in the .travis.yaml file
ARG IMG_USER=project8
ARG IMG_REPO=p8compute_dependencies
ARG IMG_TAG=v1.0.0

FROM ${IMG_USER}/${IMG_REPO}:${IMG_TAG} as psyllid_common

ARG build_type=Release
ENV PSYLLID_BUILD_TYPE=$build_type

ARG PSYLLID_TAG=beta
ENV PSYLLID_TAG=${PSYLLID_TAG}
ARG PSYLLID_BASE_PREFIX=/usr/local/p8/psyllid
ARG PSYLLID_BUILD_PREFIX=${PSYLLID_BASE_PREFIX}/${PSYLLID_PREFIX_TAG}
ENV PSYLLID_BUILD_PREFIX=${PSYLLID_BUILD_PREFIX}
ARG PSYLLID_BUILD_TYPE=RELEASE

ARG CC_VAL=gcc
ENV CC=${CC_VAL}
ARG CXX_VAL=g++
ENV CXX=${CXX_VAL}

RUN mkdir -p $PSYLLID_BUILD_PREFIX &&\
    chmod -R 777 $PSYLLID_BUILD_PREFIX/.. &&\
    cd $PSYLLID_BUILD_PREFIX &&\
    echo "source ${COMMON_BUILD_PREFIX}/setup.sh" > setup.sh &&\
    echo "export PSYLLID_TAG=${PSYLLID_TAG}" >> setup.sh &&\
    echo "export PSYLLID_BUILD_PREFIX=${PSYLLID_BUILD_PREFIX}" >> setup.sh &&\
    echo 'ln -sfT $PSYLLID_BUILD_PREFIX $PSYLLID_BUILD_PREFIX/../current' >> setup.sh &&\
    echo 'export PATH=$PSYLLID_BUILD_PREFIX/bin:$PATH' >> setup.sh &&\
    echo 'export LD_LIBRARY_PATH=$PSYLLID_BUILD_PREFIX/lib:$LD_LIBRARY_PATH' >> setup.sh &&\
    echo 'export LD_LIBRARY_PATH=$PSYLLID_BUILD_PREFIX/lib64:$LD_LIBRARY_PATH' >> setup.sh &&\
    /bin/true

########################
FROM psyllid_common as psyllid_done

COPY external /tmp_source/external
COPY monarch /tmp_source/monarch
COPY sandfly /tmp_source/sandfly
COPY source /tmp_source/source
COPY CMakeLists.txt /tmp_source/CMakeLists.txt
COPY .git /tmp_source/.git
COPY .gitmodules /tmp_source/.gitmodules
COPY PsyllidConfig.cmake.in /tmp_source/PsyllidConfig.cmake.in

## store cmake args because we'll need to run twice (known package_builder issue)
## use EXTRA_CMAKE_ARGS to add or replace options at build time, CMAKE_CONFIG_ARGS_LIST are defaults
ARG EXTRA_CMAKE_ARGS=""
ENV CMAKE_CONFIG_ARGS_LIST="\
      -D CMAKE_BUILD_TYPE=$PSYLLID_BUILD_TYPE \
      -D CMAKE_INSTALL_PREFIX:PATH=$PSYLLID_BUILD_PREFIX \
      -D Psyllid_ENABLE_FPA=FALSE \
      ${EXTRA_CMAKE_ARGS} \
      ${OS_CMAKE_ARGS} \
      "

RUN source $PSYLLID_BUILD_PREFIX/setup.sh \
    && mkdir -p /tmp_source/build \
    && cd /tmp_source/build \
    && cmake ${CMAKE_CONFIG_ARGS_LIST} .. \
    && cmake ${CMAKE_CONFIG_ARGS_LIST} .. \
    && make install \
    && /bin/true

########################
FROM psyllid_common

# for now we must grab the extra dependency content as well as psyllid itself
COPY --from=psyllid_done $PSYLLID_BUILD_PREFIX $PSYLLID_BUILD_PREFIX

# for a psyllid container, we need the environment to be configured, this is not desired for compute containers
# (in which case the job should start with a bare shell and then do the exact setup desired)
ENTRYPOINT ["/bin/bash", "-l", "-c"]
CMD ["bash"]
RUN ln -s $PSYLLID_BUILD_PREFIX/setup.sh /etc/profile.d/setup.sh

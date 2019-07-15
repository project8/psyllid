FROM project8/p8compute_dependencies:v0.2.0_psyllid as psyllid_common

ENV PSYLLID_TAG=v1.9.1
ENV PSYLLID_BUILD_PREFIX=/usr/local/p8/psyllid/$PSYLLID_TAG

RUN mkdir -p $PSYLLID_BUILD_PREFIX &&\
    chmod -R 777 $PSYLLID_BUILD_PREFIX/.. &&\
    cd $PSYLLID_BUILD_PREFIX &&\
    echo "source ${COMMON_BUILD_PREFIX}/setup.sh" > setup.sh &&\
    echo "export PSYLLID_TAG=${PSYLLID_TAG}" >> setup.sh &&\
    echo "export PSYLLID_BUILD_PREFIX=${PSYLLID_BUILD_PREFIX}" >> setup.sh &&\
    echo 'ln -sfT $PSYLLID_BUILD_PREFIX $PSYLLID_BUILD_PREFIX/../current' >> setup.sh &&\
    echo 'export PATH=$PSYLLID_BUILD_PREFIX/bin:$PATH' >> setup.sh &&\
    echo 'export LD_LIBRARY_PATH=$PSYLLID_BUILD_PREFIX/lib:$LD_LIBRARY_PATH' >> setup.sh &&\
    /bin/true

########################
FROM psyllid_common as psyllid_done

COPY dripline-cpp /tmp_source/dripline-cpp
COPY external /tmp_source/external
COPY midge /tmp_source/midge
COPY monarch /tmp_source/monarch
COPY source /tmp_source/source
COPY CMakeLists.txt /tmp_source/CMakeLists.txt
COPY .git /tmp_source/.git

# repeat the cmake command to get the change of install prefix to set correctly (a package_builder known issue)
RUN mkdir -p /tmp_source/build
RUN source $PSYLLID_BUILD_PREFIX/setup.sh &&\
    cd /tmp_source/build &&\
    cmake .. &&\
    cmake \
      -D CMAKE_INSTALL_PREFIX:PATH=$PSYLLID_BUILD_PREFIX \
      -D Psyllid_ENABLE_FPA=FALSE \
      .. &&\
    #cmake -D CMAKE_INSTALL_PREFIX:PATH=$PSYLLID_BUILD_PREFIX -D Psyllid_ENABLE_FPA=FALSE .. &&\
    make -j3 install &&\
    /bin/true

########################
FROM psyllid_common

COPY --from=psyllid_done $PSYLLID_BUILD_PREFIX $PSYLLID_BUILD_PREFIX

# for a psyllid container, we need the environment to be configured, this is not desired for compute containers
# (in which case the job should start with a bare shell and then do the exact setup desired)
ENTRYPOINT ["/bin/bash", "-l", "-c"]
CMD ["bash"]
RUN ln -s $PSYLLID_BUILD_PREFIX/setup.sh /etc/profile.d/setup.sh

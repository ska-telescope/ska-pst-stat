ARG STAT_BUILDER_IMAGE=""
ARG SMRB_RUNTIME_IMAGE=""
ARG STAT_RUNTIME_IMAGE=""

FROM $SMRB_RUNTIME_IMAGE as smrb-runtime

FROM $STAT_BUILDER_IMAGE as builder

ENV DEBIAN_FRONTEND noninteractive
ENV LD_LIBRARY_PATH=/usr/local/lib/

# Compilation
COPY . /mnt/
WORKDIR /mnt/

RUN apt update -y \
    && make local-apt-install-dependencies

RUN make local-cpp-clean-buildpath \
    && make local-cpp-build-debug \
    && make local-cpp-test-ctest \
    && make local-cpp-clean-buildpath \
    && make local-cpp-build-release \
    && cd build && make install

FROM $STAT_RUNTIME_IMAGE AS stat-runtime

ARG UID=1000
ARG GID=1000
ARG UNAME=pst

RUN groupadd -g $GID $UNAME  && \
    useradd -m -u $UID -g $GID -s /bin/bash $UNAME

ENV DEBIAN_FRONTEND noninteractive
ENV LD_LIBRARY_PATH=/usr/local/lib/

WORKDIR /usr/local
COPY --from=builder /mnt/dependencies/runtime.apt.txt ./share/
COPY --from=builder /mnt/src/ska/pst/stat/tests/data/* ./share/
COPY --from=builder /usr/local/bin/ska_pst_stat* ./bin/
COPY --from=builder /usr/local/lib/libska_pst*.so ./lib/
COPY --from=builder /usr/local/lib/libpsr*.so ./lib/
COPY --from=builder /usr/local/lib/libprotobuf*.so* ./lib/
COPY --from=builder /usr/local/lib/libgrpc*.so* ./lib/
COPY --from=builder /usr/local/lib/libabsl*.so* ./lib/
COPY --from=builder /usr/local/lib/libaddress_sorting.so* ./lib/
COPY --from=builder /usr/local/lib/libcrypto.so* ./lib/
COPY --from=builder /usr/local/lib/libgpr.so* ./lib/
COPY --from=builder /usr/local/lib/libre2.so* ./lib/
COPY --from=builder /usr/local/lib/libssl.so* ./lib/
COPY --from=builder /usr/local/lib/libupb.so* ./lib/
COPY --from=builder /usr/local/lib/libz.so* ./lib/


# RUNTIME DEPENDENCIES and CLEANUP
RUN stat share/runtime.apt.txt \
    && apt-get update -y \
    && apt-get install --no-install-recommends -y $(cat share/runtime.apt.txt) \
    && apt-get autoremove --yes && rm -rf /var/lib/apt/lists/*

# RUNTIME TEST
WORKDIR /usr/local/bin

USER $UNAME

RUN ska_pst_stat_file_proc -h \
    && ska_pst_stat_info -h

CMD ["/bin/bash"]

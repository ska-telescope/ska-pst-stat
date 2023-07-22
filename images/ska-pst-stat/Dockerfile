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
    && apt install -y libhdf5-dev

RUN make local-cpp-clean-buildpath \
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

# RUNTIME TEST
WORKDIR /usr/local/bin

USER $UNAME
CMD ["/bin/bash"]
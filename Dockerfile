FROM ubuntu:22.04

# 1. Non-interactive install, and add necessary repositories and tools
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    software-properties-common \
    build-essential \
    cmake \
    git \
    && \
    apt-get update
RUN apt install -y libboost-filesystem-dev libboost-thread-dev libboost-regex-dev libtool
RUN git clone https://github.com/encryptogroup/circuitPSU.git
WORKDIR /circuitPSU
RUN chmod +x ./setup.sh
RUN ./setup.sh
RUN ls libs/securejoin/lib
RUN chmod +x ./build.sh
RUN ./build.sh
CMD ["/bin/bash"]

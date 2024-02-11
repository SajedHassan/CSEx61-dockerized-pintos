FROM --platform=linux/amd64 ubuntu:20.04

# more secure than installing sudo
USER root

VOLUME /root/pintos

ARG DEBIAN_FRONTEND=noninteractive

# Install dependencies and QEMU
RUN apt-get update \
  && apt-get upgrade -y gcc \
  && apt-get install -y build-essential \
  binutils \
  pkg-config \
  zlib1g-dev \
  libglib2.0-dev \
  libfdt-dev \
  libpixman-1-dev \
  gcc \
  libc6-dev \
  autoconf \
  libtool \
  libsdl1.2-dev \
  g++ \
  libx11-dev \
  libxrandr-dev \
  libxi-dev \
  libnfs-dev \
  libiscsi-dev \
  valgrind \
  ninja-build \
  && apt-get upgrade -y perl \
  && apt-get install -y libc6-dbg \
  gdb \
  make \
  git \
  qemu-system-x86 \
  && apt-get clean \
  && apt-get autoclean \
  && rm -rf /var/lib/apt/* /var/lib/cache/* /var/lib/log/*

# Link qemu executable
RUN ln -s /usr/bin/qemu-system-x86_64 /bin/qemu

# Check that Qemu has been installed correctly.
RUN QEMU_BINARY=$(which qemu) \
    && if [ -z "$QEMU_BINARY" ]; then \
           echo "Error: QEMU binary not found!"; \
           exit 1; \
       fi \
    && echo "QEMU binary path: $QEMU_BINARY"

# Install PintOS
ENV PATH "/root/pintos/src/utils:$PATH"
RUN echo "export PATH=${PATH}" >> /root/.bashrc

CMD chmod -R 777 /root/pintos && bash

FROM ubuntu:22.04

# Noninteractive to avoid tzdata prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies for ns-3.36.1
RUN apt update && apt install -y \
    g++ python3 python3-dev pkg-config sqlite3 cmake python3-setuptools git \
    qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools \
    gir1.2-goocanvas-2.0 python3-gi python3-gi-cairo python3-pygraphviz \
    gir1.2-gtk-3.0 ipython3 openmpi-bin openmpi-common openmpi-doc libopenmpi-dev \
    autoconf cvs bzr unrar gsl-bin libgsl-dev libgslcblas0 wireshark tcpdump \
    sqlite sqlite3 libsqlite3-dev libxml2 libxml2-dev libc6-dev libc6-dev-i386 \
    libclang-dev llvm-dev automake python3-pip libboost-all-dev \
    && apt clean && rm -rf /var/lib/apt/lists/*

# Optional: install pybindgen via pip (used for Python bindings)
RUN pip3 install pybindgen

# Clone ns-3.36.1
WORKDIR /root
RUN git clone https://gitlab.com/nsnam/ns-3-dev.git ns-3.36.1 && \
    cd ns-3.36.1 && git checkout ns-3.36.1

# Default shell
CMD ["/bin/bash"]


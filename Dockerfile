FROM ubuntu:20.04

RUN apt-get update -y -qq && \
    apt-get install -y -qq software-properties-common && \
    apt-get install -y -qq ninja-build make gcc-multilib g++-multilib libssl-dev wget openjdk-8-jdk zip maven unixodbc-dev libc6-dev-i386 lib32readline6-dev libssl-dev libcurl4-gnutls-dev libexpat1-dev gettext unzip build-essential checkinstall libffi-dev curl libz-dev openssh-client git clang-format libasan6 ccache && \
    add-apt-repository ppa:deadsnakes/ppa && \
    apt-get update -y -qq

# Install CMake 3.21
RUN wget https://github.com/Kitware/CMake/releases/download/v3.21.3/cmake-3.21.3-linux-x86_64.sh && \
    chmod +x cmake-3.21.3-linux-x86_64.sh && \
    ./cmake-3.21.3-linux-x86_64.sh --skip-license --prefix=/usr/local && \
    cmake --version && \
    rm cmake-3.21.3-linux-x86_64.sh

# Install OpenSSL 1.1.1
RUN wget https://www.openssl.org/source/openssl-1.1.1c.tar.gz && \
    tar -xf openssl-1.1.1c.tar.gz && \
    cd openssl-1.1.1c && \
    ./config --prefix=/usr/local/ssl --openssldir=/usr/local/ssl no-shared zlib-dynamic && \
    make && \
    make install && \
    echo "/usr/local/ssl/lib" > /etc/ld.so.conf.d/openssl-1.1.1c.conf && \
    ldconfig -v && \
    cd .. && rm -rf openssl-1.1.1c*

# Install Target Version of Python
ARG PYTHON_VERSION
RUN test -n "$PYTHON_VERSION" && \
    apt-get install -y -qq python${PYTHON_VERSION}-dev python${PYTHON_VERSION} libpython${PYTHON_VERSION} python${PYTHON_VERSION}-distutils python${PYTHON_VERSION}-venv && \
    curl https://bootstrap.pypa.io/get-pip.py | python${PYTHON_VERSION} && \
    python$PYTHON_VERSION -m pip install --upgrade pip && \
    python$PYTHON_VERSION -m venv --help



# Setup the development environment
ARG UID=1000
ARG GID=1000
ARG USER=user

# Create a group and user
RUN groupadd -g $GID $USER && \
    useradd -u $UID -g $USER -s /bin/bash -m $USER

# Set the user for the subsequent Dockerfile commands
USER $USER

# Set the working directory
WORKDIR /home/$USER

RUN mkdir -p /home/$USER/development/duckdb-python-udf/ && \
    git config --global --add safe.directory /home/$USER/development/duckdb-python-udf/

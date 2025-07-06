FROM ubuntu:latest

ENV TZ=Asia/Seoul LANG=ko_KR.UTF-8 LANGUAGE=ko_KR.UTF-8

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    locales tzdata build-essential sudo gdb vim curl git wget \
    python3 cmake make gcc sudo valgrind telnet net-tools iproute2\
    && locale-gen ko_KR.UTF-8 && update-locale LANG=ko_KR.UTF-8 \
     build-essential \
    gdb \
    gcc \
    g++ \
    make \
    vim \
    git \
    man \
    net-tools \
    iputils-ping \
    curl \
    wget \
    locales \
    sudo \
    valgrind \
    strace \
    python3 \
    python3-pip \
    libc6-dev \
    libssl-dev \
    libpthread-stubs0-dev \
    libx11-dev \
    libxext-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev

RUN useradd -m -s /bin/bash jungle && usermod -aG sudo jungle
RUN echo "jungle ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/jungle
# 암호 설정
# RUN echo 'jungle:jungle' | chpasswd

#한글 에러방지용
RUN locale-gen en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

USER jungle
WORKDIR /home/jungle

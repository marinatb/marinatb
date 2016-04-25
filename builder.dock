FROM ubuntu:15.10
RUN apt-get update && apt-get install -y \
  vim-nox \
  tmux \
  build-essential \
  g++ \
  gdb \
  clang \
  lldb \
  libc++-dev \
  libc++abi-dev \
  cmake \
  ninja-build \
  ssh \
  rsync \
  git \
  curl \
  libssl-dev \
  libboost-dev \
  sudo

RUN mkdir /build

ADD 3p /3p

RUN cd /3p/proxygen/proxygen && \
  ./deps.sh && \
  ./reinstall.sh
  
  

ENTRYPOINT ["ninja"]

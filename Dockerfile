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
  libboost-dev

RUN mkdir /build

ENTRYPOINT ["ninja"]

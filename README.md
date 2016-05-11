# MarinaTB
A network testbed for cybersecurity experimentation.

## Building

Right now we are setup to run on Ubuntu 16.04 LTS.

### Build Environment Setup

```shell
cd deps
./ubuntu-tools.sh
./all-libs.sh
```

### Building marinatb

```shell
#get 3p sources
git submodule init
git submodule update

mkdir build
cd build
cmake .. -G Ninja
ninja
```

# builder
Builder of the testbed

## Building

### Build Environment Setup

```shell
cd deps
./ubuntu-tools.sh
./all-libs.sh
```

### Building marinatb

```shell
mkdir build
cd build
cmake .. -G Ninja
ninja
```

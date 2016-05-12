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

### Running tests (requires docker)
```shell
./builder containerize-system   #creates containers for all system components
./builder net                   #creates a network that interconnects component containers
./builder launch-system         #launches the test system
./builder containerize test     #containerizes the test container (also hooked up to test network)
./builder run-console test      #get a console into the test container

#run the tests within the container
root@test:/# /code/build/test/api/run_api_tests [api-blueprint]
root@test:/# /code/build/test/api/run_api_tests [api-blueprint]
root@test:/# exit

#tear down the test system
./builder terminate-system
```

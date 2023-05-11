# Tillitis hello worlds


## Prerequisites


### General dependencies

```bash
sudo apt install -y \
                 build-essential clang lld llvm bison flex libreadline-dev \
                 gawk tcl-dev libffi-dev git mercurial graphviz \
                 xdot pkg-config python3 libftdi-dev \
                 python3-dev libeigen3-dev \
                 libboost-dev libboost-filesystem-dev \
                 libboost-thread-dev libboost-program-options-dev \
                 libboost-iostreams-dev cmake libusb-1.0-0-dev \
                 ninja-build libglib2.0-dev libpixman-1-dev \
                 golang clang-format

# NOTE, this might be necessary if clang-15 is not default (e.g. Ubuntu 22.04)
#   sudo apt install -y clang-15 lldb-15 lld-15
#   sudo update-alternatives --install /usr/bin/clang      clang       /usr/bin/clang-15  200
#   sudo update-alternatives --install /usr/bin/ld.lld     ld.lld      /usr/bin/ld.lld-15  200
#   sudo update-alternatives --install /usr/bin/llvm-ar    llvm-ar     /usr/bin/llvm-ar-15 200
#   sudo update-alternatives --install /usr/bin/llvm-objcopy  llvm-objcopy /usr/bin/llvm-objcopy-15 200
```

### Tillitis

```bash
cd ..

# build runapp (for transfering our app to tkey)
git clone https://github.com/tillitis/tillitis-key1-apps
cd tillitis-key1-apps
make tkey-runapp
cd ..


# build libraries (using in linking, see Makefile)
git clone https://github.com/tillitis/tkey-libs
cd tkey-libs
make all
cd ..
```

## Start app

Start application on tkey using tkey-runapp


```
make

../tillitis-key1-apps/tkey-runapp coin_race.bin
```



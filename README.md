# Tillitis hello worlds

The TKey is a fully open source hardware token, licensed under copyleft.

Its FPGA code and firmware is locked down, and user applications are only stored in RAM: cleared when disconnected.
Therefore, it has no state and user applications can only access the unique device identifier.

This identifier can be used to verify that the device or application has not been modified since last run.
Additionally, it can be used for key derivation. For instance, an ssh-agent is provided by Tillitis in the apps repo [1].


The TKey has several use cases, and applications are in active development.
It is also easy to write apps for the TKey using regular C (Tillitis provides a limited standard lib).

- It can provide good random data.
Sign..
2FA requiring click..
....

For instance, it is quite simple to use the ssh-agent app in TKey to create a 2FA step in PAM authentication.
That is, use the TKey for Linux login (for instance).
I provide an example of this in here: [tkey_authentication](tkey_authentication.md)

[1] - <https://github.com/tillitis/tillitis-key1-apps>.


## Running an app on TKey

It is quick to build an app and transfer it to TKey.

Run the following commands to try the app in this repo, or other example apps in <https://github.com/tillitis/tillitis-key1-apps>.

### Prerequisites


#### General dependencies

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

# NOTE, clang >= 15 required, might need to be set manually if not default (e.g. on Ubuntu 22.04)
#   sudo apt install -y clang-15 lldb-15 lld-15
#   sudo update-alternatives --install /usr/bin/clang      clang       /usr/bin/clang-15  200
#   sudo update-alternatives --install /usr/bin/ld.lld     ld.lld      /usr/bin/ld.lld-15  200
#   sudo update-alternatives --install /usr/bin/llvm-ar    llvm-ar     /usr/bin/llvm-ar-15 200
#   sudo update-alternatives --install /usr/bin/llvm-objcopy  llvm-objcopy /usr/bin/llvm-objcopy-15 200
```

#### Tillitis

```bash
# build libraries (used in linking, see Makefile)
git clone https://github.com/tillitis/tkey-libs
cd tkey-libs
make all
cd ..

# build runapp (for transfering our app to tkey)
git clone https://github.com/tillitis/tillitis-key1-apps
cd tillitis-key1-apps
make tkey-runapp
cd ..
```


```bash
# Give active user permission to interact with the device,
#   for instance add user to dialout group

#device is usually here and owned by group dialout
ls -l /dev/ttyACM0

#add user to dialout
sudo usermod -a -G dialout $USER

#relogin or run this command
newgrp dialout
```


### Start app


Start application on tkey using tkey-runapp. Simply run the tkey-runapp binary and provide the app you'd like to transfer.

The example app coin_race.c simply uses the internal TRNG to "flip a coin", either red or blue wins the toss.
The current leader is shown using the LED, white signifies an equal position.


```
make

../tillitis-key1-apps/tkey-runapp coin_race.bin
```



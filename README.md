# Tillitis - a first look

The TKey is a handy open source security token created by the Mullvad spin-off [Tillitis AB](https://tillitis.se/).
Both its software and hardware (schematics, PCB, and FPGA design) is open source, and available under copyleft licenses [1].
The TKey was released in April of 2023 with a price tag of 880 SEK (incl. tax) [2].

The TKey is a small device drawing less than 100mA. It provides a few basic functions (either in firmware or by the provided apps/libs [3]) such as Ed25519 signing, key derivation, BLAKE2 hashing, and a good random number generator (TRNG).
It also has a sensor for user confirmation.
With these features it is possible to use the TKey for different functionality, such as authentication (signing, SSH login, passkey, ..), as a root of trust (sign/encrypt), or as a RNG.

The firmware is locked down and user applications need to be loaded onto the TKey each boot, as it keeps no user-state between power cycles.
However, it has an internal unique device identifier which can be used to derive keys persistent across boots, in addition to verify that the device and application have not been modified since last boot.
See <https://dev.tillitis.se> for more info.


Applications are in active development, but it is also possible to write apps for the TKey using regular C (Tillitis provides a limited standard lib).
I wrote a quick "hello world"-like example [coin_race.c](coin_race.c), build instructions below.

Moreover, an idea I got, is to use the ssh-agent app in TKey to create a 2FA step in PAM authentication.
That is, to use the TKey for Linux login (for instance).
I provide examples of this in here, [tkey_authentication.md](tkey_authentication.md), where I use the TKey to authenticate to `sudo`.

[1] - <https://github.com/tillitis/tillitis-key1> \
[2] - <https://shop.tillitis.se/products/tkey> \
[3] - <https://github.com/tillitis/tillitis-key1-apps>, <https://github.com/tillitis/tkey-libs>

## Running an app on TKey

It is quick to build an app and transfer it to the TKey.

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



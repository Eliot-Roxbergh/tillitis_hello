# Tillitis - a first look

The TKey is an open source security token created by the Mullvad spin-off [Tillitis AB](https://tillitis.se/).
Both its software and hardware (schematics, PCB, and FPGA design) are open source, and available under copyleft licenses [1].
The TKey was released in April of 2023 with a price tag of 880 SEK (incl. tax) [2].

The TKey is a small device drawing less than 100mA. It provides a few basic functions (either in firmware or by the provided apps/libs [3]) such as Ed25519 signing, key derivation, BLAKE2 hashing, and a good random number generator (TRNG).
It also has a sensor for user confirmation.
With these features, the TKey can be used for various functionalities, such as authentication (signing, SSH login, passkey, etc.), serving as a root of trust (sign/encrypt), or as a source of entropy (TRNG/CSPRNG).

The firmware is locked down and user applications need to be loaded onto the TKey after each boot, as it keeps no user-state between power cycles.
However, it has an internal Unique Device Secret (UDS) which can be used to derive keys that are persistent across reboots, in addition to verify that the device and application have not been modified since last boot.
For more technical details, see <https://dev.tillitis.se>.

Tillitis includes a few applications, but it is also simple to write apps for the TKey.
Apps are currently written in C and Tillitis provides a limited standard lib and basic functionality such as TRNG, cryptographic operations (hashing, signing, and key derivation), support for the touch sensor, and two-way communication with a client application on the host computer. [3]
I wrote a quick "hello world"-like example [coin_race.c](coin_race.c), build instructions below.
For more examples, see _tillitis-key1-apps/apps/*.c_ [3].

Moreover, an idea I got, is to use the ssh-agent app in TKey to create a 2FA step in PAM authentication.
That is, to use the TKey for Linux login (for instance).
I describe this here, [tkey_authentication.md](tkey_authentication.md), where I use the TKey to authenticate to `sudo`.


[1] - <https://github.com/tillitis/tillitis-key1> \
[2] - <https://shop.tillitis.se/products/tkey> \
[3] - <https://github.com/tillitis/tillitis-key1-apps>, <https://github.com/tillitis/tkey-libs>

## Running an app on TKey

It is quick to build an app and transfer it to the TKey.

Run the following commands to try the app in this repo, or other example apps in <https://github.com/tillitis/tillitis-key1-apps>.

### Prerequisites

#### Install build dependencies

- Install dependencies

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

- Clone and build TKey libraries and apps

```bash
# build libraries (used in linking, see Makefile)
git clone https://github.com/tillitis/tkey-libs
cd tkey-libs
make all
cd ..

# build runapp (for transferring our app to tkey)
git clone https://github.com/tillitis/tillitis-key1-apps
cd tillitis-key1-apps
make apps
make tkey-runapp
chmod +x tkey-runapp
cd ..
```

- Get access to the TKey device

```bash
# Give the active user permission to interact with the device,
#   for instance by adding them to the dialout group

#TKey is usually found here and owned by group dialout
ls -l /dev/ttyACM0

#add user to dialout
sudo usermod -a -G dialout $USER

#relogin or run this command
newgrp dialout
```


### Transfer app

Transfer the application to the tkey using tkey-runapp. Simply run the tkey-runapp binary and provide the app you'd like to transfer.
Once transferred, the application will automatically start on the TKey.

The example app I wrote, [coin_race.c](coin_race.c), simply uses the internal TRNG to "flip a coin", either red or blue wins the toss.
The current leader is shown using the LED, white signifies an equal position.


```
make

../tillitis-key1-apps/tkey-runapp coin_race.bin
```



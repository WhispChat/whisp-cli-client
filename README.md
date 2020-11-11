# whisp-cli-client
This repository contains the code for the command line interface client for the
Whisp chat platform.

## Prerequisites
- CMake >=3.12
- Make
- C++17 compiler
- [Google Protocol Buffers](https://developers.google.com/protocol-buffers) >=3.13.0
- [GTK](https://www.gtk.org/) >=3.0
- [gktmm](https://www.gtkmm.org/) >=3.0

> **⚠ IMPORTANT**  
>If you're using an X server for graphic displays, setting the following environment variables is required:
>* export DISPLAY=$(grep -m 1 nameserver /etc/resolv.conf | awk '{print $2}'):0.0
>* export LIBGL_ALWAYS_INDIRECT=1
>
>If you're using WSL2, use the following arguments when starting the X server:
>* -wgl -ac

## Installation
1. Clone repository:
```bash
git clone --recurse-submodules git@github.com:WhispChat/whisp-cli-client.git
```
2. Create build directory:
```bash
mkdir build
```
3. Enter build directory and build code:
```bash
cd build/
cmake .. && make -j
```

This will create the binary `whisp-cli` in the `build/src` directory.

## Connecting and Testing
To use this client to test and connect to a Whisp server, first build and run
the `whisp-server` binary from the [server repository](https://github.com/WhispChat/whisp-server).
When the server is running, you can run the `whisp-cli` binary to connect to the
server. You should see the message `[INFO] Connected to 0.0.0.0:8080`. You can
start multiple client processes to test message broadcasting and such.

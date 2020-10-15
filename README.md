# whisp-cli-client
This repository contains the code for the command line interface client for the
Whisp chat platform.

## Prerequisites
- CMake >=3.12
- Make
- C++17 compiler

> **⚠ WARNING**  
>When building in a Windows environment, make sure that you have the Windows SDK installed.
>For example, the Windows SDK for Windows 10 can be found at:
>https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk/

## Installation
1. Clone repository:
```bash
git clone git@github.com:WhispChat/whisp-cli-client.git
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

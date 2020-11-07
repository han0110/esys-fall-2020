# BLE GATT 101

![DEMO](./demo.gif)

This work shows how to implements a simple BLE GATT server with corresponding client to interact with, where the GATT server has 3 services and 1 characteristic per service:

- LED Service - It provides a writable characteristic which controls the digital out of `LED1`.
- Button Service - It provides a readable/notification characteristic which notifies subscribers when `USER_BUTTON` pressed or released.
- Student ID Service - It provides a readable value which responses `bxx901xxx`.

## Table of Content

- [BLE GATT 101](#ble-gatt-101)
  - [Table of Content](#table-of-content)
  - [Prerequisite](#prerequisite)
  - [Run](#run)
    - [GATT Server](#gatt-server)
    - [GATT Client](#gatt-client)

## Prerequisite

- Development Device
  - [Mbed Studio](https://os.mbed.com/studio) - To compiles and flashes GATT server program into MCU
- GATT Client Device
  - Tool
    - [CMake](https://cmake.org) - To help the compilation process for GATT client
  - C Library
    - [`libglib2.0-dev`](https://packages.ubuntu.com/en/bionic/libglib2.0-dev) - Dependency of GATT client
    - [`libreadline-dev`](https://packages.ubuntu.com/en/bionic/libreadline-dev) - Dependency of `gattlib`
    - [`libbluetooth-dev`](https://packages.ubuntu.com/en/bionic/libbluetooth-dev) - Dependency of `gattlib`
    - [`gattlib`](https://github.com/labapart/gattlib) - Dependency of GATT client

## Run

### GATT Server

Use [Mbed Studio](https://os.mbed.com/studio) to open workspace for directory `gatt-server`, then compile with [`mbed-os@6.4.0`](https://github.com/ARMmbed/mbed-os/tree/8ef0a435b2356f8159dea8e427b2935d177309f8) and flash into MCU STM32L475.

### GATT Client

On Raspberry Pi 3, compile the GATT client:

```shell
cd gatt-client
mkdir build && pushd build && cmake .. && make && popd
```

Execute:

```shell
./build/gatt-client
```

After execution it will do the following things:

- Discover the GATT server with name `BLE GATT 101`
- Connect to it
- Write `led_state` characteristic value to `true` to make `LED1` on MCU to glow
- Read `student_id` characteristic value and print to stdout
- Subscribe to `button` characteristic, which will print the notification when `USER_BUTTON` on MCU pressed or released
- Wait for `SIGTERM`
- Close connection and exit the process

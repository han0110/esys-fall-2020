# Wi-Fi 101

![DEMO](./demo.gif)

This work shows how to connect to Internet by Wi-Fi and send the acceleration information and `USER_BUTTON` state from MCU to server by TCP. The server serves web application static files and pipe the TCP stream data from MCU to web client by WebSocket, which controls the white sphere to move and emit laser beam.

## Table of Content

- [Wi-Fi 101](#wi-fi-101)
  - [Table of Content](#table-of-content)
  - [Prerequisite](#prerequisite)
  - [Run](#run)
    - [Accelero Client](#accelero-client)
    - [Accelero Server](#accelero-server)
  - [Reference](#reference)

## Prerequisite

- Development Device
  - [Mbed Studio](https://os.mbed.com/studio) - To compiles and flashes Accelero client program into MCU
- Accelero Server Device
  - Tool
    - [Node](https://nodejs.org) - To run Accelero server

## Run

### Accelero Client

Use [Mbed Studio](https://os.mbed.com/studio) to open workspace for directory `accelero-client`, then compile with [`mbed-os@6.4.0`](https://github.com/ARMmbed/mbed-os/tree/8ef0a435b2356f8159dea8e427b2935d177309f8) and flash into MCU STM32L475.

After started it will do the following things:

- Connect to Wi-Fi AP whose SSID and password defined in [`mbed_app.json`](./accelero-client/mbed_app.json)
- Connect to Accelero server whose address and port also defined in [`mbed_app.json`](./accelero-client/mbed_app.json)
- Start to send acceleration information and `USER_BUTTON` state every 30 milliseconds.

### Accelero Server

On Raspberry Pi 3, build the web application static files:

```shell
cd ./accelero-server
npm i && npm run build
```

Execute:

```shell
npm run start
```

After execution it will do the following things:

- Listen TCP on port `8080`
- Listen HTTP on port `8081`
- When TCP connection established from MCU, grab it and wait for WebSocket connection.
- When WebSocket connection established from web client, pipe the data from TCP socket to WebSocket socket.

## Reference

- [three.js webgl - postprocessing - unreal bloom selective](https://github.com/mrdoob/three.js/blob/master/examples/webgl_postprocessing_unreal_bloom_selective.html)

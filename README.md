# BLE Temperature Sensor

## Overview

This is a skeleton for an Apache Mynewt project implementing a temperature sensor.
You will need the Apache Newt tool, as documented in the [Getting Started Guide](http://mynewt.apache.org/os/get_started/introduction/).

## Code

The source files are located in [apps/ble_temp_sensor](apps/ble_temp_sensor):

  * pkg.yml contains the base definition of the app
  * syscfg.yml contains setting definitions and overrides
  * src/main.c is the main entry point of the application
  * src/temp.c implements the temperature sensing functionality
  * src/gatt_svr.c implements the basic BLE GATT server functionality

The available targets are specified in [targets/](targets/).

## Build

1. Download and install Apache Newt tool.

2. Download the Apache Mynewt core and package dependencies:

```no-highlight
    $ newt install
```

3. To build a given application target:

```no-highlight
    $ newt build <target>
```

4. To create an image suitable for flashing a target board:

```no-highlight
    $  newt create-image <target> 1.0.0
```

## Run

To flash the target board:

```no-highlight
    $  newt load <target>
```

Here is an `openocd` command to flash an image onto the board manually, if you prefer
to know how the sausage is made (you could also use JLink to similar effect):

```no-highlight
    $ openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg \
        -c 'init' \
        -c 'reset halt' \
        -c 'flash write_image erase bin/targets/ble_temp_sensor/app/apps/ble_temp_sensor/ble_temp_sensor.img 0x8000 bin' \
        -c 'reset' \
        -c 'exit'
```

Here is some sample serial console output from a typical client session:

```no-highlight
000000 [ts=0ssb, mod=64 level=0] registered service 0x1800 with handle=1
000001 [ts=7812ssb, mod=64 level=0] registered characteristic 0x2a00 with def_handle=2 val_handle=3
000004 [ts=31248ssb, mod=64 level=0] registered characteristic 0x2a01 with def_handle=4 val_handle=5
000008 [ts=62496ssb, mod=64 level=0] registered characteristic 0x2a05 with def_handle=7 val_handle=8
000011 [ts=85932ssb, mod=64 level=0] registered service 5c3a659e-897e-45e1-b016-007107c96df6 with handle=10
000013 [ts=101556ssb, mod=64 level=0] registered characteristic 5c3a659e-897e-45e1-b016-007107c96df7 with def_handle=11 val_handle=12
000018 [ts=140616ssb, mod=64 level=1] adv started
000679 [ts=5304668ssb, mod=64 level=1] connection established; status=0
001394 [ts=10890568ssb, mod=64 level=1] read value=2875
001468 [ts=11468720ssb, mod=64 level=1] read value=2875
001543 [ts=12054684ssb, mod=64 level=1] read value=2875
001578 [ts=12328104ssb, mod=64 level=1] read value=2850
001970 [ts=15390600ssb, mod=64 level=1] disconnect; reason=531
```

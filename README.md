# BLE Temperature Sensor

## Overview

This is a modification of the skeleton for an Apache Mynewt project implementing a temperature sensor.
It implements a BLE temperature sensor that samples every 100 msec.  When reading the characteristic, it will will return the last ten samples (1 sec) of the temperature readings. The format is 16 bit little endian ascii encoded hex.

## Code

The skeleton files were modified as follows

  * src/main.c initialize the temperature history mechanism and start a 100 msec timer.
  * src/temp.c record the temperature every timer tick, and initiate a new timer.
  * src/gatt_svr.c implements the basic BLE GATT server functionality (no modifications)

  * added a bootloader target 

The available targets are specified in [targets/](targets/).

## Build

newt build nano2_boot
newt build nano2_ble_temp

newt create-image nano2_ble_temp 1.0.0

## Run

To flash the target board:

```no-highlight
    $  newt load nano2_boot
    $  newt load nano2_ble_temp
```

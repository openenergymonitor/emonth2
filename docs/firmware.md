# Firmware

The emonTH2 firmware is based on Arduino.

## Available Firmware

### [emonTh2](https://github.com/openenergymonitor/emonth2/tree/master/firmware):

**New:** Support for three different radio formats is available: JeeLib Classic, JeeLib Native and **LowPowerLabs**.

## Updating firmware using an emonPi/emonBase

The easiest way of updating the emonTh2 firmware is to connect it to an emonPi or emonBase with a USB to UART cable and then use the firmware upload tool available at `Setup > Admin > Update > Firmware`.

The example images below show the earlier [Wicked Device / OpenEnergyMonitor Programmer](../electricity-monitoring/programmers/wicked-device.md). The programmer is the small board that plugs in to the emonTh2 on the 6-pin UART header. The [newer programmer](../electricity-monitoring/programmers/ftdi-programmer.md) currently available in the shop needs to be orientated the other way around. **Make sure that GND on the programmer matches up with GND on the emonTh2 board.**

Refresh the update page after connecting the USB cable. You should now see port ttyUSB0 appear in the ‘Select port` list.

*emonPi to emonTh2 programmer image*

Select port: `ttyUSB0`, Hardware: `emonTh2`, Radio format e.g: `RFM69 LowPowerLabs`:

*emonTh firmware upload image*

Click `Update Firmware` to upload the firmware.

## How to compile and upload firmware

### PlatformIO Command Line

PlatformIO works great from the command line. See the excellent [PlatformIO Quick Start Guide](https://docs.platformio.org/en/latest/core/installation/index.html#super-quick-mac-linux) for installation instructions.

**Compile and upload emonTh2 firmware**

    git clone https://github.com/openenergymonitor/emonth2
    cd emonth2/firmware
    pio run -t upload

**View serial port with PlatformIO CLI**

    pio device monitor

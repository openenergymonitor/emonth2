# emonTH V2.0

[![Build Status](https://travis-ci.org/openenergymonitor/emonth2.svg?branch=master)](https://travis-ci.org/openenergymonitor/emonth2)

**Low power wireless temperature and humidity node**

*V2.0 hardware revision adds support for SI7021 temperature and humidity. This sensor brings performance and power savings over the DHT22, see [sensor](sensor) folder of this repo for sensor evaluation and comparison.*

The emonTH V2 is an open-source, wireless, battery powered temperature and humidity monitoring node.

Data from the emonTH is transmitted via wireless RF (433MHz) to an emonPi / emonBase web-connected base-station for logging to Emoncms for data logging, processing and graphing.


![emonTH V2](docs/img/emonth2.png)

## Documentation

- [Overview](https://docs.openenergymonitor.org/emonth2/overview.html)
- [Installation](https://docs.openenergymonitor.org/emonth2/install.html)
- [Technical](https://docs.openenergymonitor.org/emonth2/technical.html)

## Hardware

- Hardware schematic & CAD files are in the `hardware` folder of this repo

## Firmware

### Pre-compiled 

Either upload pre-compiled firmware (from github releases) using our [emonUpload tool](https://github.com/openenergymonitor/emonupload) or compile and upload

### Compile & Upload

PlatformIO is recommended as the easiest way to compile and upload Arduino compatible firmware for emonTH. One of the big advantages of using PlatformIO is all libraries can be automatically downloaded. Arduino IDE can also be used but libraries must be manually installed.

### Compile & Upload Firmware Using PlatformIO

For a full guide and installing and using PlatformIO (including visual IDE) see ['Compiling' User Guide Section](https://guide.openenergymonitor.org/technical/compiling). Assuming platformIO is installed

```
$ git clone https://github.com/openenergymonitor/emonth2
$ cd emonth2
$ pio run
$ pio -t upload
```

## Development

The emonTH 2.0 builds on the earlier emonTH v1.x: https://github.com/openenergymonitor/emonth.

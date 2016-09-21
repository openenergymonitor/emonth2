# emonTH V2.0

**Low power wireless temperature and humidity node**

V2.0 hardware revision adds support for SI7021 temperature and humidity. This sensor brings performance and power savings over the DHT22, see `sensor_test` folder of this repo for sensor evaluation and comparison. 

- BLOG POST
- FORUM POST


# Firmware

PlatformIO is recommended as the easiest way to compile and upload Arduino compatible firmware for emonTH. One of the big advantages of using PlatformIO is all libraries can be automatically downloaded. Arduino IDE can also be used but libraries must be manually installed.

## Compile & Upload Firmware Using PlatformIO

For a full guide and installing and using PlatformIO (including visual IDE) see ['Compiling' User Guide Section](https://guide.openenergymonitor.org/technical/compiling). Assuming platformIO is installed

```
$ git clone https://github.com/openenergymonitor/emonth2
$ cd emonth2
$ pio run
$ pio -t upload
```

# Hardware

emonTH hardware is fully open-source. See `hardware` folder of this repo for Gerbers, Eagle CAD design, BOM & port map.

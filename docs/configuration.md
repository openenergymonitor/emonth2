# Configuration

- Battery operation is expected, although 5 V d.c. can be supplied via the FTDI connector or the screw terminal block if desired.
- Transmits readings from internal temperature and humidity sensor, also external DS18B20 temperature sensor can be connected if desired.
- Settings can be adjusted at power-up using the FTDI connector and a serial programmer.

## Setting up

Data is transmitted by radio approximately every 58 s, identified as Node 23. Changing the internal DIP switches before power-up can change this to Node 24, 25 or 26.

Adding an external temperature sensor before power-up will allow it to be recognised and the readings automatically included in the sent data.

## Additional set-up options

At power-up, if you have a serial monitor connected via the FTDI connector and set to 115200 baud with “both NL & CR” selected, you will see a “Welcome” message giving the software version number, the radio, pulse counter and temperature settings, and some more diagnostic messages. At the end is an invitation to enter the access code **+++**. You have a 5 s window to enter that (or you may ignore it, when after the delay, the emonTH will start up, you will see nothing more and you will not be able to change anything until you restart the sketch).

If you successfully enter the access code, you will see an advisory and a short menu:

Entering Settings mode...

Available commands:

- **l** list the settings
- **r** restore sketch defaults
- **s** save settings to EEPROM
- **v** show firmware version
- **x** exit, lock and continue
- **?** show this text again

- **w\<x\>** turn RFM Wireless data off: x = 0 or on: x = 1
- **b\<n\>** set r.f. band n = a single numeral: 4 = 433MHz, 8 = 868MHz, 9 = 915MHz (may require hardware change)
- **p\<nn\>** set the r.f. power. nn - an integer 0 - 31 representing -18 dBm to +13 dBm. Default: 25 (+7 dBm)
- **g\<nnn\>** set Network Group nnn - an integer (OEM default = 210)
- **n\<nn\>** set node ID n= an integer (standard node ids are 1..60)

- **m\<x\> \<yy\>** meter pulse counting:
  - x = 0 for OFF, x = 1 for ON, 
  - \<yy\> = an integer for the pulse minimum period in ms. (y is not needed, or ignored when x = 0)

- **t0 \<y\>** turn external temperature measurement on or off: y = 0 for OFF, y = 1 for ON

- **t\<x\> \<yy\> \<yy\> \<yy\> \<yy\> \<yy\> \<yy\> \<yy\> \<yy\>**
  - change an external temperature sensor's address or position:
  - x = a single numeral: the position of the sensor in the list (1-based)
  - yy = 8 hexadecimal bytes representing the sensor's address
  - e.g. 28 81 43 31 07 00 00 D
  - N.B. Sensors CANNOT be added.

Only the radio, pulse and temperature sensor settings can be changed, the temperature and humidity sensors cannot be calibrated. Normally, you should save ‘ **s’** the settings before you exit ‘ **x’** , so that they will be retained and used forever (until changed again).

If you turn the radio off and serial data on ( **w2** ), only the serial data in a format compatible with the emonHub Serial Interfacer will be sent to the FTDI port. ( **w3** ) will send data both by radio and the serial port, the default is radio only ( **w1** ).

If you turn the radio power up above the default value of 25, ensure you select the frequency band that matches the radio module fitted, else the radio module itself could be destroyed. Turning the power up will significantly reduce the battery life, conversely turning the power down will increase the battery life.

If you change the NodeID, then the internal DIP switches add 0, 1, 2 or 3 to the new NodeID.

The meter pulse minimum period inhibits the effect of ‘contact bounce’ if the pulses come from a mechanical switch. The period can be lengthened if necessary, or shortened to zero if the pulses come from an electronic switch.

The external temperature sensor is only turned on if one is detected at the start. A connected sensor can be turned off to save power without needing to disconnect it.

If you need two or more sensors, then although the sketch supports up to four, it will be necessary to edit and recompile the sketch to include the additional readings in the sent data. The ability to manually edit the sensor addresses will allow the order of the sensors to be preserved in the event that one has been replaced. If a replacement sensor is not recognised, make a note of the addresses of all, set the address of the first to “00 00 00 00 00 00 00 00” and restart the sketch. You can then manually re-enter the addresses in the order you require and save the settings.

## Data Output

The output by radio is, in order:

1. Internal temperature
2. External temperature
3. Humidity
4. Battery voltage
5. Pulse Count.

The “key:value” pairs serial output for the EmonESP & EmonHubOEMInterfacer sends the following

1. Internal temperature temp
2. External temperature tempex
3. Humidity humidity
4. Battery voltage batt
5. Pulse Count pulse

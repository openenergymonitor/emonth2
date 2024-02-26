/*
  emonTH V2 Low Power SI7021 Humidity & Temperature, DS18B20 Temperature & Pulse counting Node Example

  Si7021 = internal temperature & Humidity
  DS18B20 = External temperature


  Part of the openenergymonitor.org project
  Licence: GNU GPL V3

  Authors: Glyn Hudson
  Builds upon JCW JeeLabs RF12 library, Arduino and Martin Harizanov's work

  THIS SKETCH REQUIRES:

  Libraries required:
   - see platform io.ini
   - recommend compiling with platformIO for auto library download https://guide.openenergymonitor.org/technical/compiling
   - Arduino IDE can be used to compile but libs will need to be manually downloaded

  Recommended node ID allocation
  -----------------------------------------------------------------------------------------------------------
  -ID-	-Node Type-
  0	- Special allocation in JeeLib RFM12 driver - reserved for OOK use
  1-4     - Control nodes
  5-10	- Energy monitoring nodes
  11-14	--Un-assigned --
  15-16	- Base Station & logging nodes
  17-30	- Environmental sensing nodes (temperature humidity etc.)
  31	- Special allocation in JeeLib RFM12 driver - Node31 can communicate with nodes on any network group
  -------------------------------------------------------------------------------------------------------------
  */

const char *firmware_version = {"4.1.6"};
/*

  Change log:
  V4.1.6   - (26/02/24) Fix DS18B20 serial printing & multiple sensors 
  V4.1.5   - (21/08/23) Fix node ID DIP switch selection 
  V4.1.4   - (21/07/23) Serial print RF format  
  V4.1.3   - (05/06/23) Startup serial print streamline for factory test 
  V4.1.2   - (27/02/23) Add option to used custom encrpytion key
  V4.1.1   - (19/02/23) Fix missing frequency initialization
  V4.1.0   - (17/02/23) LowPowerLabs radio format option
  V4.0.0   - (10/07/21) Replace JeeLib with OEM RFM69nTxLib using RFM69 "Native" packet format, add emonEProm library support
  
  V3.2.4   - (25/05/18) Add prompt for serial config
  V3.2.3   - (17/07/17) Fix DIP switch had no effect
  V3.2.2   - (12/05/17) Fix DIP switch nodeID not being read when EEPROM is configures
  V3.2.1   - (30/11/16) Fix emonTx port typo
  V3.2.0   - (13/11/16) Run-time serial nodeID config
  V3.1.0   - (19/10/16) Test for RFM69CW and SI7021 at startup, allow serial use without RF prescent
  V3.0.0   - (xx/10/16) Add support for SI7021 sensor instead of DHT22 (emonTH V2.0 hardware)
  ^^^ emonTH V2.0 hardware ^^^
  V2.7   - (15/09/16) Serial print serial pairs for emonesp compatiable e.g. temp:210,humidity:56
  V2.6   - (24/10/15) Tweek RF transmission timmng to help reduce RF packet loss
  V2.5   - (23/10/15) default nodeID 23 to enable new emonHub.conf decoder for pulseCount packet structure
  V2.4   - (15/10/15) activate pulse count pin input pullup to stop spurious pulses when no sensor connected
  v2.3.1 - (12/10/14) don't flash LED on RF transmission to save power
  v2.3   - rebuilt based on low power pulse counting code by Eric Amann: http://openenergymonitor.org/emon/node/10834
  v2.2   - 60s RF transmit period now uses timer1, pulse events are decoupled from RF transmit
  v2.4   - 5 min default transmisson time = 300 ms
  v2.1   - Branched from emonTH_DHT22_DS18B20 example, first version of pulse counting version
 -------------------------------------------------------------------------------------------------------------
  emonhub.conf node decoder:
  See: https://github.com/openenergymonitor/emonhub/blob/emon-pi/configuration.md

    [[23]]
      nodename = emonTH_5
      firmware = V2.x_emonTH_DHT22_DS18B20_RFM69CW_Pulse
      hardware = emonTH_(Node_ID_Switch_DIP1:OFF_DIP2:OFF)
      [[[rx]]]
         names = temperature, external temperature, humidity, battery, pulseCount
         datacodes = h,h,h,h,L
         scales = 0.1,0.1,0.1,0.1,1
         units = C,C,%,V,p
  */
// -------------------------------------------------------------------------------------------------------------

// #define ATTINY                                                      // Uncomment for attiny

#define RFM69_JEELIB_CLASSIC 1
#define RFM69_JEELIB_NATIVE 2
#define RFM69_LOW_POWER_LABS 3

// Set in platformio.ini
//#define RadioFormat RFM69_LOW_POWER_LABS

bool flash_led = false;                                                // true = Flash LED after each sample (increases battery drain)

// These variables control the transmit timing of the emonTH
const unsigned long WDT_PERIOD = 80;                                   // mseconds.
const unsigned long WDT_MAX_NUMBER = 690;                              // Data sent after WDT_MAX_NUMBER periods of WDT_PERIOD ms without pulses:
                                                                       // 690 × 80 = 55.2 seconds (it needs to be about 5s less than the record interval in emoncms)
const unsigned long PULSE_MAX_NUMBER = 100;                            // Data sent after PULSE_MAX_NUMBER pulses
#define RFM69CW                                                        // Use the RFM69CW radio

#if RadioFormat == RFM69_LOW_POWER_LABS
  #include <RFM69.h>                             // RFM69 LowPowerLabs radio library
#elif RadioFormat == RFM69_JEELIB_CLASSIC
  #include <rfmTxLib.h>                          // RFM69 transmit-only library using "JeeLib RFM69 Native" message format
#elif RadioFormat == RFM69_JEELIB_NATIVE
  #include <rfm69nTxLib.h>                       // RFM69 transmit-only library using "JeeLib RFM69 Native" message format
#endif

#include <emonEProm.h>                                                 // OEM EEPROM library
#ifndef ATTINY
  #include <JeeLib.h>
#endif
#include <avr/power.h>
#include <avr/sleep.h>
#include <OneWire.h>
#include <DallasTemperature.h>
// Include EmonTH_V2_config.ino in the same directory as this for configuration functions & data


#define REG_SYNCVALUE1 0x2F
#define MAX_SENSORS 4                                                  // The maximum number of external temperature sensors.
                                                                                                                                      
#define EXTERNAL_TEMP_SENSORS 1                                        // Specify number of external temperature sensors connected                                                                      
                                                                       
#define FACTORYTESTGROUP 1                                             // Transmit the Factory Test on Grp 1 
                                                                       //   to avoid interference with recorded data at power-up.

//---------------------------- emonTH Settings - Stored in EEPROM and shared with config.ino ------------------------------------------------
struct 
{
  byte RF_freq = RF69_433MHZ;                                          // Frequency of radio module can be RF69_433MHZ, RF69_868MHZ or RF69_915MHZ. 
  byte networkGroup = 210;                                             // Wireless network group, must be the same as emonBase / emonPi and emonGLCD. OEM default is 210
  byte  nodeID = 23;                                                   // Node ID for this sensor.
  byte  rf_on = 1;                                                     // RF/Serial output. Bit 0 set: RF on, bit 1 set: serial on.
  byte  rfPower = 25;                                                  // 0 - 31 = -18 dBm (min value) - +13 dBm (max value). RFM12B equivalent: 25 (+7 dBm)
                                                                       //    lower power means increased battery life
  bool  pulse_enable = true;                                           // Pulse counting
  int   pulse_period = 50;                                             // Pulse min period - 0 = no de-bounce
  bool  temperatureEnabled = true;                                     // Enable external temperature measurement
  DeviceAddress allAddresses[MAX_SENSORS];                             // External sensor address data
} EEProm;

uint16_t eepromSig = 0xFF06;                                           // 'Experimental' signature - see oemEProm Library documentation for details.
bool calibration_enable = false;                                       // Start with calibration disabled
byte nodeID = EEProm.nodeID;
const int busyThreshold = -97;                                         // Signal level below which the radio channel is clear to transmit
const byte busyTimeout = 0;                                            // Time in ms to wait for the channel to become clear, before transmitting anyway. Set to zero to
                                                                       //   inhibit channel occupancy check (conserves battery life but increases the risk of lost messages)
                                                                       //   Recommended value when used: 15 (ms)


#if RadioFormat == RFM69_LOW_POWER_LABS
  RFM69 radio;
#endif

const int TEMPERATURE_PRECISION = 11;                                  // DS18B20 resolution 9,10,11 or 12bit corresponding to (0.5, 0.25, 0.125, 0.0625 degrees C LSB),
                                                                       // lower resolution means lower power
                                                                       // 9 (93.8ms), 10 (187.5ms), 11 (375ms) or 12 (750ms) bits equal to resolution of 0.5C, 0.25C, 0.125C and 0.0625C
#define ASYNC_DELAY 375                                                // 9bit requires 95ms, 10bit 187ms, 11bit 375ms and 12bit resolution takes 750ms
// See block comment above for library info
ISR(WDT_vect) { 
  #ifndef ATTINY
  Sleepy::watchdogEvent(); 
  #endif
}                             // Attach JeeLib sleep function to Atmega328 watchdog - enables MCU to be put into sleep mode inbetween readings to reduce power consumption

// SI7021_status SPI temperature & humidity sensor
#include <Wire.h>
#include <SI7021.h>
SI7021 SI7021_sensor;
bool SI7021_status;

// Hardwired emonTH pin allocations
const byte DS18B20_PWR =    5;
const byte LED =            9;
const byte BATT_ADC =       1;
const byte DIP_switch1 =    7;
const byte DIP_switch2 =    8;
const byte pulse_count_pin =3;                                         // INT 1 / Dig 3 Screw Terminal Block Number 4
#define ONE_WIRE_BUS       17
const byte DHT22_PWR =      6;                                         // Not used in emonTH V2.0, 10K resistor R1 connects DHT22 pins
const byte DHT22_DATA =    16;                                         // Not used in emonTH V2.0, 10K resistor R1 connects DHT22 pins.

bool dip1;                                                             // DIP switch state
bool dip2;                                                             // DIP switch state

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Note: Please update emonhub configuration guide on OEM wide packet structure change:
// https://github.com/openenergymonitor/emonhub/blob/emon-pi/configuration.md
typedef struct 
{                                                                      // RFM RF payload datastructure
  int temp;
  int temp_external[EXTERNAL_TEMP_SENSORS];
  int humidity;
  int battery;
  unsigned long pulsecount;
} Payload;
Payload emonth;

int numSensors;                                                        // The number of external DS18B20 sensors found

volatile unsigned long pulseCount;
unsigned long WDT_number;
bool newPulse;

unsigned long now, start;
const byte SLAVE_ADDRESS = 42;


static void showString (PGM_P s);

//################################################################################################################################
//################################################################################################################################
#ifndef UNIT_TEST // IMPORTANT LINE! // http://docs.platformio.org/en/stable/plus/unit-testing.html

void setup() 
{
//################################################################################################################################

  pinMode(LED,OUTPUT); digitalWrite(LED,HIGH);                         // Status LED on

  // Unused pins configure as input pull up for low power
  // http://electronics.stackexchange.com/questions/43460/how-should-unused-i-o-pins-be-configured-on-atmega328p-for-lowest-power-consumpt
  // port map: https://github.com/openenergymonitor/emonth2/blob/master/hardware/readme.md
  pinMode(DHT22_PWR, INPUT_PULLUP);                                    // DHT22 not used on emonTH V2.
  pinMode(DHT22_DATA, INPUT_PULLUP);                                   // DHT22 not used on emonTH V2
  pinMode(14, INPUT_PULLUP);
  pinMode(20, INPUT_PULLUP);
  pinMode(21, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);

  //READ DIP SWITCH POSITIONS - LOW when switched on (default off - pulled up high)
  pinMode(DIP_switch1, INPUT_PULLUP);
  pinMode(DIP_switch2, INPUT_PULLUP);
  dip1 = digitalRead(DIP_switch1);
  dip2 = digitalRead(DIP_switch2);

  Serial.begin(115200);
  Serial.println("OpenEnergyMonitor.org");
  Serial.print("emonTH FW: V"); Serial.println(firmware_version);
  delay(100);

  load_config(true);                                                   // Load RF config from EEPROM (if any exists)
  
  if (EEProm.rf_on)
  {
    list_calibration();
    
#ifdef RFM69CW
    #if RadioFormat == RFM69_LOW_POWER_LABS
      Serial.println("RadioFormat: LowPowerLabs");
    #elif RadioFormat == RFM69_JEELIB_CLASSIC
      Serial.println("RadioFormat: JeeLib Classic");
    #elif RadioFormat == RFM69_JEELIB_NATIVE
      Serial.println("RadioFormat: JeeLib Native");
    #endif
    Serial.println("Init RFM...");
    #if RadioFormat == RFM69_LOW_POWER_LABS
      radio.initialize(EEProm.RF_freq,EEProm.nodeID,EEProm.networkGroup);  
      #ifndef RFM69_LPL_AES_ENCRYPTION_KEY
      radio.encrypt("89txbe4p8aik5kt3");                                                      // initialize RFM
      #else
      radio.encrypt(RFM69_LPL_AES_ENCRYPTION_KEY);                                            // initialize RFM
      #endif
      radio.setPowerLevel(EEProm.rfPower);
    #else
      rfm_init();                                                        // initialize RFM
    #endif
#endif

    Serial.println("RFM Started");

    // Send RFM69CW test sequence (for factory testing)
    for (int i = 10; i> -1; i--)
    {
      emonth.temp=i;
      #if RadioFormat == RFM69_LOW_POWER_LABS
        // radio.send(5, (const void*)(&emonth), sizeof(emonth));
        // radio.sleep();
      #else
        rfm_send((byte *)&emonth, sizeof(emonth), FACTORYTESTGROUP, nodeID, EEProm.RF_freq, EEProm.rfPower, busyThreshold, busyTimeout);
      #endif
      delay(100);
    }
    emonth.temp = 0;
    // end of factory test sequence
  }

  pinMode(DS18B20_PWR,OUTPUT);
  pinMode(BATT_ADC, INPUT);
  pinMode(pulse_count_pin, INPUT_PULLUP);

  //################################################################################################################################
  // Setup and for presence of si7021
  //################################################################################################################################
  Serial.println("Int SI7021..");

  // check if the I2C lines are HIGH
  if (digitalRead(SDA) == HIGH || digitalRead(SCL) == HIGH)
  {
    SI7021_sensor.begin();
    int deviceid = SI7021_sensor.getDeviceId();
    if (deviceid!=0) 
    {
      SI7021_status=1;
      si7021_env data = SI7021_sensor.getHumidityAndTemperature();
      Serial.print("SI7021 Started, ID: ");
      Serial.println(deviceid); delay(100);
      Serial.print("temp:"); Serial.print(data.celsiusHundredths/100.0); Serial.print(",humidity:"); Serial.println(data.humidityBasisPoints/100.0);
      delay(100);
    }
    else 
    {
      SI7021_status=0;
      Serial.println("SI7021 Error");
    }
  }
  else
    Serial.println("SI7021 Error");

  //################################################################################################################################
  // Setup and for presence of DS18B20
  //################################################################################################################################
  digitalWrite(DS18B20_PWR, HIGH); delay(50);
  sensors.begin();
  sensors.setWaitForConversion(false);                                 // disable automatic temperature conversion to reduce time spent awake, 
                                                                       // conversion will be implemented manually in sleeping 
                                                                       // http://harizanov.com/2013/07/optimizing-ds18b20-code-for-low-power-applications/

  if (EEProm.allAddresses[0][0] == 0x00)                               // No sensor addresses recorded in EEPROM
  {
    numSensors = (sensors.getDeviceCount()); 
    if (numSensors > MAX_SENSORS)
      numSensors = MAX_SENSORS;
  }
  else
  {
    numSensors = MAX_SENSORS;
    for (numSensors = MAX_SENSORS; numSensors > 0; numSensors--)
    {
      if (EEProm.allAddresses[numSensors-1][0] == 0x28)                // 0x28 = signature of a DS18B20, so a pre-existing sensor
          break;
    }
  }
  byte j=0;                                                            // search for one wire devices and copy to device address array.
  if (EEProm.allAddresses[0][0] != 0x28)                               // 0x28 = signature of a DS18B20, so a pre-existing array - do not search for sensors
    while ((j < numSensors) && (oneWire.search(EEProm.allAddresses[j]))) 
      j++;
  
  
  // Inital read from DS18B20
  if (numSensors && EEProm.temperatureEnabled)
  {
    digitalWrite(DS18B20_PWR, HIGH); dodelay(50);
    for(int j=0;j<numSensors;j++) 
       sensors.setResolution(EEProm.allAddresses[j], TEMPERATURE_PRECISION);      // and set the a to d conversion resolution of each.
       sensors.requestTemperatures();                                             // Send the command to get temperatures
       dodelay(ASYNC_DELAY);                                                      //Must wait for conversion, since we use ASYNC mode
      
    for(int j=0;j<EXTERNAL_TEMP_SENSORS;j++) {
       float temp=(sensors.getTempC(EEProm.allAddresses[j]));
       if ((temp < 125.0) && (temp > -40.0)) {
         emonth.temp_external[j] = (temp*10);
       }
     }
      digitalWrite(DS18B20_PWR, LOW);
   }
  
  // Print inital DS18B20 sensor readings 
  if (numSensors)
  {
    printTemperatureSensorAddresses();
    for(int j=0;j<EXTERNAL_TEMP_SENSORS;j++) {
      Serial.print("tempex");Serial.print(j);Serial.print(":");Serial.print(emonth.temp_external[j]); Serial.println(",");
    }
  }


  //################################################################################################################################
  // Config mode
  //################################################################################################################################

  getSettings();
  
  //################################################################################################################################
  // Interrupt pulse counting setup
  //################################################################################################################################

  startPulseCount();
  
  
  //################################################################################################################################
  // Power Save  - turn off what we don't need - http://www.nongnu.org/avr-libc/user-manual/group__avr__power.html
  //################################################################################################################################
  #ifndef ATTINY
  ACSR |= (1 << ACD);                                                  // disable Analog comparator
  if (!(EEProm.rf_on & 0x02)) power_usart0_disable();                  // disable serial UART
  power_twi_disable();                                                 // Two Wire Interface module:
  power_spi_disable();
  power_timer1_disable();
  // power_timer0_disable();                                           //don't disable necessary for the DS18B20 library
  #endif
  //turn off pullups on dip switches set to 0V (50k-20k pullup so 66uA-165uA per pin at 3.3V)
  if (!dip1)
    pinMode(DIP_switch1, INPUT);
  if (!dip2)
    pinMode(DIP_switch2, INPUT);

  // Only turn off LED if sensor is working
  if (SI7021_status)
  {
    digitalWrite(LED,LOW);                                             // turn off LED to indicate end setup
  }
} // end of setup


//################################################################################################################################
//################################################################################################################################
void loop()
//################################################################################################################################
{
  if (newPulse) {
    #ifndef ATTINY
    Sleepy::loseSomeTime(EEProm.pulse_period);
    #endif
    newPulse = false;
  }

  #ifndef ATTINY
  if (Sleepy::loseSomeTime(WDT_PERIOD)==1) {
    WDT_number++;
  }
  #endif

  if (WDT_number>=WDT_MAX_NUMBER || pulseCount>=PULSE_MAX_NUMBER)
  {
    if (EEProm.pulse_enable)
    {
      cli();
      emonth.pulsecount += pulseCount;
      pulseCount = 0;
      sei();
    }


    if (numSensors && EEProm.temperatureEnabled)
    {
      digitalWrite(DS18B20_PWR, HIGH); dodelay(50);
      for(int j=0;j<numSensors;j++) 
        sensors.setResolution(EEProm.allAddresses[j], TEMPERATURE_PRECISION);      // and set the a to d conversion resolution of each.
      sensors.requestTemperatures();                                   // Send the command to get temperatures
      dodelay(ASYNC_DELAY);                                            //Must wait for conversion, since we use ASYNC mode
      
      for(int j=0;j<EXTERNAL_TEMP_SENSORS;j++) {
        float temp=(sensors.getTempC(EEProm.allAddresses[j]));
        if ((temp < 125.0) && (temp > -40.0)) {
          emonth.temp_external[j] = (temp*10);
        }
      }
      digitalWrite(DS18B20_PWR, LOW);
    }

    emonth.battery=int(analogRead(BATT_ADC)*0.0322);                   //read battery voltage, convert ADC to volts x10

    //Enhanced battery monitoring mode. In this mode battery values
    //sent in x*1000 mode instead of x*10. This allows to have more accurate
    //values on emonCMS x.xx instead of x.x
    // NOTE if you are going to enable this mode you need to
    // 1. Disable x*10 mode. By commenting line above.
    // 2. Change multiplier in line 353 Serial.print(emonth.battery/10.0);
    // 3. Change scales factor in the emonhub node decoder entry for the emonTH
    // See more https://community.openenergymonitor.org/t/emonth-battery-measurement-accuracy/1317
    //emonth.battery=int(analogRead(BATT_ADC)*3.222);

    // Read SI7021
    // Read from SI7021 SPI temp & humidity sensor
    if (SI7021_status==1)
    {
      #ifndef ATTINY
      power_twi_enable();
      #endif
      si7021_env data = SI7021_sensor.getHumidityAndTemperature();
      emonth.temp = (data.celsiusHundredths*0.1);
      emonth.humidity = (data.humidityBasisPoints*0.1);
      #ifndef ATTINY
      power_twi_disable();
      #endif
    }


    // Send data via RF
    if (EEProm.rf_on & 0x01)
    {
      #ifndef ATTINY
      power_spi_enable();
      #endif
      dodelay(30);                                                     // wait for module to wakup
      #if RadioFormat == RFM69_LOW_POWER_LABS
        radio.setAddress(nodeID);
        radio.send(5, (const void*)(&emonth), sizeof(emonth));
        radio.sleep();
      #else
        rfm_send((byte *)&emonth, sizeof(emonth), EEProm.networkGroup, nodeID, EEProm.RF_freq, EEProm.rfPower, busyThreshold, busyTimeout);
      #endif
      dodelay(100);
      #ifndef ATTINY
      power_spi_disable();
      #endif
    }

    if (flash_led)
    {
      digitalWrite(LED,HIGH);
      dodelay(100);
      digitalWrite(LED,LOW);
    }


    if (EEProm.rf_on & 0x02)
    {
      // Serial print strings pairs e.g. "temp:2634,humidity:4010,batt:33"
      // Works with EmonESP direct serial
      Serial.print("temp:");Serial.print(emonth.temp); Serial.print(",");

      if (numSensors)
      {
        for(int j=0;j<EXTERNAL_TEMP_SENSORS;j++) {
          Serial.print("tempex");Serial.print(j);Serial.print(":");Serial.print(emonth.temp_external[j]); Serial.print(",");
        }
      }

      if (SI7021_status)
      {
        Serial.print("humidity:");Serial.print(emonth.humidity); Serial.print(",");
      }
      Serial.print("batt:"); Serial.print(emonth.battery);
      if (emonth.pulsecount > 0) 
      {
        Serial.print(",");
        Serial.print("pulse:"); Serial.print(emonth.pulsecount);
      }
      Serial.println();
      delay(5);
    } // end serial print


    now = millis();
    WDT_number=0;
  } // end WDT

} // end loop

void dodelay(unsigned int ms)
{
  #ifndef ATTINY
  byte oldADCSRA=ADCSRA;
  byte oldADCSRB=ADCSRB;
  byte oldADMUX=ADMUX;

  Sleepy::loseSomeTime(ms); // JeeLabs power save function: enter low power mode for x seconds (valid range 16-65000 ms)

  ADCSRA=oldADCSRA;         // restore ADC state
  ADCSRB=oldADCSRB;
  ADMUX=oldADMUX;
  #endif
}

  //################################################################################################################################
  // Interrupt pulse counting setup
  //################################################################################################################################
void startPulseCount(void)
{
  emonth.pulsecount = 0;
  pulseCount = 0;
  WDT_number=720;
  newPulse = false;
  attachInterrupt(digitalPinToInterrupt(pulse_count_pin), onPulse, RISING);
}

void stopPulseCount(void)
{
  detachInterrupt(digitalPinToInterrupt(pulse_count_pin));
}

// The interrupt routine - runs each time a rising edge of a pulse is detected
void onPulse()
{
  newPulse = true;                           // flag for new pulse set to true
  pulseCount++;                              // number of pulses since the last RF sent
}

void printTemperatureSensorAddresses(void)
{
  DeviceAddress *temperatureSensors = EEProm.allAddresses;
  Serial.print(F("DS18B20 found = "));
  Serial.print(numSensors);
  Serial.print(" of ");
  Serial.println(MAX_SENSORS);
  
  if (numSensors)
  {
      Serial.println(F(", with addresses..."));
      for (int j=0; j< numSensors; j++)
      {
          for (int i=0; i<8; i++)
          {
              if (temperatureSensors[j][i] < 0x10)
                Serial.print(F("0"));
              Serial.print(temperatureSensors[j][i], 16);
              Serial.print(F(" "));
          }
          if (temperatureSensors[j][6] == 0x03)
            Serial.print(F("Sensor may not be reliable"));
          Serial.println();
          delay(5);
      }
  }
  // Serial.println();
  // Serial.print(F("DS18B20 measurement is"));
  // Serial.print(EEProm.temperatureEnabled?"":" NOT");
  // Serial.println(F(" enabled."));
  delay(5);
        
}


#endif    // IMPORTANT LINE! end unit test
//http://docs.platformio.org/en/stable/plus/unit-testing.html

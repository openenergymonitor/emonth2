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
   - see platformio.ini
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
  Change log:
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
boolean debug=1;                                                      // Set to 1 to few debug serial output
boolean flash_led=0;                                                  // Flash LED after each sample (battery drain) default=0

const unsigned int  version = 324;                                    // firmware version
// These variables control the transmit timing of the emonTH
const unsigned long WDT_PERIOD = 80;                                  // mseconds.
const unsigned long WDT_MAX_NUMBER = 690;                             // Data sent after WDT_MAX_NUMBER periods of WDT_PERIOD ms without pulses:
                                                                      // 690x 80 = 55.2 seconds (it needs to be about 5s less than the record interval in emoncms)
const  unsigned long PULSE_MAX_NUMBER = 100;                          // Data sent after PULSE_MAX_NUMBER pulses
const  unsigned long PULSE_MAX_DURATION = 50;


#define RF69_COMPAT 1                                                 // Set to 1 if using RFM69CW or 0 is using RFM12B
#include <JeeLib.h>                                                   // https://github.com/jcw/jeelib
#include <RF69_avr.h>
#define REG_SYNCVALUE1      0x2F
boolean RF_STATUS;


byte RF_freq=RF12_433MHZ;                                           // Frequency of RF12B module can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. You should use the one matching the module you have.
byte nodeID = 23;                                                      // EmonTH temperature RFM12B node ID - should be unique on network
int networkGroup = 210;                                         // EmonTH RFM12B wireless network group - needs to be same as emonBase and emonGLCD
                                                                      // DS18B20 resolution 9,10,11 or 12bit corresponding to (0.5, 0.25, 0.125, 0.0625 degrees C LSB),
                                                                      // lower resolution means lower power

const int TEMPERATURE_PRECISION=11;                                   // 9 (93.8ms),10 (187.5ms) ,11 (375ms) or 12 (750ms) bits equal to resplution of 0.5C, 0.25C, 0.125C and 0.0625C
#define ASYNC_DELAY 375                                               // 9bit requres 95ms, 10bit 187ms, 11bit 375ms and 12bit resolution takes 750ms
// See block comment above for library info
#include <avr/power.h>
#include <avr/sleep.h>
#include <OneWire.h>
#include <DallasTemperature.h>
ISR(WDT_vect) { Sleepy::watchdogEvent(); }                            // Attached JeeLib sleep function to Atmega328 watchdog -enables MCU to be put into sleep mode inbetween readings to reduce power consumption

// SI7021_status SPI temperature & humidity sensor
#include <Wire.h>
#include <SI7021.h>
SI7021 SI7021_sensor;
boolean SI7021_status;

// Hardwired emonTH pin allocations
const byte DS18B20_PWR=    5;
const byte LED=            9;
const byte BATT_ADC=       1;
const byte DIP_switch1=    7;
const byte DIP_switch2=    8;
const byte pulse_countINT= 1;                                        // INT 1 / Dig 3 Screw Terminal Block Number 4
const byte pulse_count_pin=3;                                        // INT 1 / Dig 3 Screw Terminal Block Number 4
#define ONE_WIRE_BUS       17
const byte DHT22_PWR=       6;                                      // Not used in emonTH V2.0, 10K resistor R1 connects DHT22 pins
const byte DHT22_DATA=      16;                                     // Not used in emonTH V2.0, 10K resistor R1 connects DHT22 pins.

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
boolean DS18B20;                                                      // create flag variable to store presence of DS18B20

// Note: Please update emonhub configuration guide on OEM wide packet structure change:
// https://github.com/openenergymonitor/emonhub/blob/emon-pi/configuration.md
typedef struct {                                                      // RFM RF payload datastructure
  int temp;
  int temp_external;
  int humidity;
  int battery;
  unsigned long pulsecount;
} Payload;
Payload emonth;

int numSensors;
//addresses of sensors, MAX 4!!
byte allAddress [4][8];                                              // 8 bytes per address

volatile unsigned long pulseCount;
unsigned long WDT_number;
boolean  p;

unsigned long now, start;
const byte SLAVE_ADDRESS = 42;

const char helpText1[] PROGMEM =                                 // Available Serial Commands
"\n"
"Available commands:\n"
"  <nn> i     - set node IDs (standard node ids are 1..30)\n"
"  <n> b      - set MHz band (4 = 433, 8 = 868, 9 = 915)\n"
"  <nnn> g    - set network group (RFM12 only allows 212, 0 = any)\n"
"  s          - save config to EEPROM\n"
"  v          - Show firmware version\n"
;
//################################################################################################################################
//################################################################################################################################
#ifndef UNIT_TEST // IMPORTANT LINE! // http://docs.platformio.org/en/stable/plus/unit-testing.html

void setup() {
//################################################################################################################################

  pinMode(LED,OUTPUT); digitalWrite(LED,HIGH);                       // Status LED on

  // Unused pins configure as input pull up for low power
  // http://electronics.stackexchange.com/questions/43460/how-should-unused-i-o-pins-be-configured-on-atmega328p-for-lowest-power-consumpt
  // port map: https://github.com/openenergymonitor/emonth2/blob/master/hardware/readme.md
  pinMode(DHT22_PWR, INPUT_PULLUP);                                  //DHT22 not used on emonTH V2.
  pinMode(DHT22_DATA, INPUT_PULLUP);                                 //DHT22 not used on emonTH V2
  pinMode(14, INPUT_PULLUP);
  pinMode(20, INPUT_PULLUP);
  pinMode(21, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);

  //READ DIP SWITCH POSITIONS - LOW when switched on (default off - pulled up high)
  pinMode(DIP_switch1, INPUT_PULLUP);
  pinMode(DIP_switch2, INPUT_PULLUP);
  boolean DIP1 = digitalRead(DIP_switch1);
  boolean DIP2 = digitalRead(DIP_switch2);


  if (debug==1)
  {
    Serial.begin(115200);
    Serial.println("OpenEnergyMonitor.org");
    Serial.print("emonTH FW: V"); Serial.println(version);
    delay(100);
  }

  // Test for RFM69CW and int with test sequence if found
  // spiInit();
  //writeReg(REG_SYNCVALUE1, 0xAA);
  // int result = readReg(REG_SYNCVALUE1); //CAUSED RFM TO HANG
  // writeReg(REG_SYNCVALUE1, 0x55);
   //result = result + readReg(REG_SYNCVALUE1);
  // readReg(REG_SYNCVALUE1);
  // if (result!=0){  // result will be > 0 if RFM69CW is found
    // RF_STATUS = 1;
  // } else {
    // if (debug) Serial.println("RFM NOT Detected");
    // RF_STATUS =0;
  // }
 RF_STATUS=1;

  if (RF_STATUS==1){
    load_config();                                                        // Load RF config from EEPROM (if any exist)

    // Add effect of DIP switch positions to nodeID
    if ((DIP1 == HIGH) && (DIP2 == HIGH)) nodeID=nodeID;
    if ((DIP1 == LOW) && (DIP2 == HIGH)) nodeID=nodeID+1;
    if ((DIP1 == HIGH) && (DIP2 == LOW)) nodeID=nodeID+2;
    if ((DIP1 == LOW) && (DIP2 == LOW)) nodeID=nodeID+3;

    if (debug) Serial.println("Int RFM...");
    rf12_initialize(nodeID, RF_freq, networkGroup);                       // Initialize RFM

    if (debug){
      Serial.println("RFM Started");
      Serial.print("Node: ");
      Serial.print(nodeID);
      Serial.print(" Freq: ");
      if (RF_freq == RF12_433MHZ) Serial.print("433Mhz");
      if (RF_freq == RF12_868MHZ) Serial.print("868Mhz");
      if (RF_freq == RF12_915MHZ) Serial.print("915Mhz");
      Serial.print(" Network: ");
      Serial.println(networkGroup);
    }
    // Send RFM69CW test sequence (for factory testing)
    for (int i=10; i>-1; i--)
    {
      emonth.temp=i;
      rf12_sendNow(0, &emonth, sizeof emonth);
      delay(100);
    }
    rf12_sendWait(2);
    emonth.temp=0;
    // end of factory test sequence
    rf12_sleep(RF12_SLEEP);
  }

  pinMode(DS18B20_PWR,OUTPUT);
  pinMode(BATT_ADC, INPUT);
  pinMode(pulse_count_pin, INPUT_PULLUP);

  //################################################################################################################################
  // Setup and for presence of si7021
  //################################################################################################################################
  if (debug==1) Serial.println("Int SI7021..");

  // check if the I2C lines are HIGH
  if (digitalRead(SDA) == HIGH || digitalRead(SCL) == HIGH)
  {
    SI7021_sensor.begin();
    int deviceid = SI7021_sensor.getDeviceId();
    if (deviceid!=0) {
      SI7021_status=1;
      if (debug){
        si7021_env data = SI7021_sensor.getHumidityAndTemperature();
        Serial.print("SI7021 Started, ID: ");
        Serial.println(deviceid);
        Serial.print("SI7021 t: "); Serial.println(data.celsiusHundredths/100.0);
        Serial.print("SI7021 h: "); Serial.println(data.humidityBasisPoints/100.0);
      }
    }
    else {
      SI7021_status=0;
      if (debug) Serial.println("SI7021 Error");
    }
  }
  else {
    SI7021_status=0;
    if (debug) Serial.println("SI7021 Error");
  }
  //################################################################################################################################
  // Setup and for presence of DS18B20
  //################################################################################################################################
  digitalWrite(DS18B20_PWR, HIGH); delay(50);
  sensors.begin();
  sensors.setWaitForConversion(false);                             //disable automatic temperature conversion to reduce time spent awake, conversion will be implemented manually in sleeping http://harizanov.com/2013/07/optimizing-ds18b20-code-for-low-power-applications/
  numSensors=(sensors.getDeviceCount());
  byte j=0;                                        // search for one wire devices and
                                                   // copy to device address arrays.
  while ((j < numSensors) && (oneWire.search(allAddress[j])))  j++;
  digitalWrite(DS18B20_PWR, LOW);

  if (numSensors==0)
  {
    if (debug==1) Serial.println("No DS18B20");
    DS18B20=0;
  }
  else
  {
    DS18B20=1;
    if (debug==1) {
      Serial.print(numSensors); Serial.println(" DS18B20");
    }
  }
  if (debug==1) delay(100);


  //################################################################################################################################
  // Interrupt pulse counting setup
  //################################################################################################################################
  emonth.pulsecount = 0;
  pulseCount = 0;
  WDT_number=720;
  p = 0;
  attachInterrupt(pulse_countINT, onPulse, RISING);

  //################################################################################################################################
  // RF Config mode
  //################################################################################################################################
  if (RF_STATUS==1){
    Serial.println("");
    Serial.println("'+++' then [Enter] for RF config mode");
    Serial.println("(Arduino IDE Serial Monitor: make sure 'Both NL & CR' is selected)");
    Serial.println("waiting 5s...");
    start = millis();
    while (millis() < (start + 5000)){
      // If serial input of keyword string '+++' is entered during 5s power-up then enter config mode
      if (Serial.available()){
        if ( Serial.readString() == "+++\r\n"){
          Serial.println("Entering config mode...");
          showString(helpText1);
          // char c[]="v"
          config(char('v'));
          while(1){
            if (Serial.available()){
              config(Serial.read());
            }
          }
        }
      }
    }
  }

  //################################################################################################################################
  // Power Save  - turn off what we don't need - http://www.nongnu.org/avr-libc/user-manual/group__avr__power.html
  //################################################################################################################################
  ACSR |= (1 << ACD);                     // disable Analog comparator
  if (debug==0) power_usart0_disable();   //disable serial UART
  power_twi_disable();                    //Two Wire Interface module:
  power_spi_disable();
  power_timer1_disable();
  // power_timer0_disable();              //don't disable necessary for the DS18B20 library

  // Only turn off LED if both sensor and RF69CW are working
  if ((RF_STATUS) && (SI7021_status)){
    digitalWrite(LED,LOW);                  // turn off LED to indciate end setup
  }
} // end of setup


//################################################################################################################################
//################################################################################################################################
void loop()
//################################################################################################################################
{

  if (p) {
    Sleepy::loseSomeTime(PULSE_MAX_DURATION);
    p=0;
  }

  if (Sleepy::loseSomeTime(WDT_PERIOD)==1) {
    WDT_number++;
  }

  if (WDT_number>=WDT_MAX_NUMBER || pulseCount>=PULSE_MAX_NUMBER)
  {
    cli();
    emonth.pulsecount += (unsigned int) pulseCount;
    pulseCount = 0;
    sei();


    if (DS18B20==1)
    {
      digitalWrite(DS18B20_PWR, HIGH); dodelay(50);
      for(int j=0;j<numSensors;j++) sensors.setResolution(allAddress[j], TEMPERATURE_PRECISION);      // and set the a to d conversion resolution of each.
      sensors.requestTemperatures();                                        // Send the command to get temperatures
      dodelay(ASYNC_DELAY); //Must wait for conversion, since we use ASYNC mode
      float temp=(sensors.getTempC(allAddress[0]));
      digitalWrite(DS18B20_PWR, LOW);
      if ((temp<125.0) && (temp>-40.0))
      {
        emonth.temp_external=(temp*10);
      }
    }

    emonth.battery=int(analogRead(BATT_ADC)*0.0322);                    //read battery voltage, convert ADC to volts x10

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
    if (SI7021_status==1){
      power_twi_enable();
      si7021_env data = SI7021_sensor.getHumidityAndTemperature();
      emonth.temp = (data.celsiusHundredths*0.1);
      emonth.humidity = (data.humidityBasisPoints*0.1);
      power_twi_disable();
    }


    // Send data via RF
    if (RF_STATUS){
      power_spi_enable();
      rf12_sleep(RF12_WAKEUP);
      dodelay(30);                                   // wait for module to wakup
      rf12_sendNow(0, &emonth, sizeof emonth);
      // set the sync mode to 2 if the fuses are still the Arduino default
      // mode 3 (full powerdown) can only be used with 258 CK startup fuses
      rf12_sendWait(2);
      rf12_sleep(RF12_SLEEP);
      dodelay(100);
      power_spi_disable();
    }

    if (flash_led){
      digitalWrite(LED,HIGH);
      dodelay(100);
      digitalWrite(LED,LOW);
    }


    if (debug==1)
    // Serial print strings pairs e.g. "temp:2634,humidity:4010,batt:33"
    // Works with EmonESP direct serial
    {
      Serial.print("temp:");Serial.print(emonth.temp); Serial.print(",");

      if (DS18B20){
        Serial.print("tempex:");Serial.print(emonth.temp_external); Serial.print(",");
      }

      if (SI7021_status){
        Serial.print("humidity:");Serial.print(emonth.humidity); Serial.print(",");
      }
      Serial.print("batt:"); Serial.print(emonth.battery);
      if (emonth.pulsecount > 0) {
        Serial.print(",");
        Serial.print("pulse:"); Serial.print(emonth.pulsecount);
      }
      Serial.println();
      delay(5);
    } // end serial print debug


    unsigned long last = now;
    now = millis();
    WDT_number=0;
  } // end WDT

} // end loop

void dodelay(unsigned int ms)
{
  byte oldADCSRA=ADCSRA;
  byte oldADCSRB=ADCSRB;
  byte oldADMUX=ADMUX;

  Sleepy::loseSomeTime(ms); // JeeLabs power save function: enter low power mode for x seconds (valid range 16-65000 ms)

  ADCSRA=oldADCSRA;         // restore ADC state
  ADCSRB=oldADCSRB;
  ADMUX=oldADMUX;
}

// The interrupt routine - runs each time a rising edge of a pulse is detected
void onPulse()
{
  p=1;                                       // flag for new pulse set to true
  pulseCount++;                              // number of pulses since the last RF sent
}

// Used to test for RFM69CW prescence
static void writeReg (uint8_t addr, uint8_t value) {
    RF69::control(addr | 0x80, value);
}

static uint8_t readReg (uint8_t addr) {
    return RF69::control(addr, 0);
}



#endif    // IMPORTANT LINE! end unit test
//http://docs.platformio.org/en/stable/plus/unit-testing.html

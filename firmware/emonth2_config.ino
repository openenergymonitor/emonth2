/*
Configuration functions for EmonTH_V2_rfm69n.ino
*/


#include <Arduino.h>
#include <avr/pgmspace.h>
#include <EEPROM.h>

#define OFF !0
#define ON 0

 // Available Serial Commands
const PROGMEM char helpText[] =                                
"\n"
"Available commands:\n"
"  l         - list the settings\n"
"  r         - restore sketch defaults\n"
"  s         - save settings to EEPROM\n"
"  v         - show firmware version\n"
"  x         - exit, lock and continue\n"
"  ?         - show this text again\n"
"\n"
"  w<x>      - turn RFM Wireless / Serial data both off: x = 0, wireless only: x=1, serial only: x=2 or both: x = 3\n"
"  b<n>      - set r.f. band n = a single numeral: 4 = 433MHz, 8 = 868MHz, 9 = 915MHz (may require hardware change)\n"
"  p<nn>     - set the r.f. power. nn - an integer 0 - 31 representing -18 dBm to +13 dBm. Default: 25 (+7 dBm)\n"
"  g<nnn>    - set Network Group  nnn - an integer (OEM default = 210)\n"
"  n<nn>     - set node ID n= an integer (standard node ids are 1..60)\n"
"\n"
"  m<x> <yy> - meter pulse counting:\n"
"               x = 0 for OFF, x = 1 for ON, <yy> = an integer for the pulse minimum period in ms. (y is not needed, or ignored when x = 0)\n"
"  t0 <y>    - turn external temperature measurement on or off:\n"
"            - y = 0 for OFF, y = 1 for ON\n"
"  t<x> <yy> <yy> <yy> <yy> <yy> <yy> <yy> <yy>\n"
"            - change an external temperature sensor's address or position:\n"
"            - x = a single numeral: the position of the sensor in the list (1-based)\n"
"            - yy = 8 hexadecimal bytes representing the sensor's address\n"
"               e.g.  28 81 43 31 07 00 00 D9\n"
"               N.B. Sensors CANNOT be added.\n"
;

extern DeviceAddress *temperatureSensors;

static void load_config(bool verbose)
{
  if (eepromRead(eepromSig, (byte *)&EEProm)) 
  {
    if (verbose)
      Serial.println(F("Loaded EEPROM config"));
    else
      Serial.println(F("No EEPROM config"));
  }
}

static void list_calibration(void)
{
  // Add effect of DIP switch positions to nodeID
  // No change if (DIP1 == OFF) && (DIP2 == OFF)
  nodeID = EEProm.nodeID & 0x3F;
  if ((dip1 == ON) && (dip2 == OFF)) nodeID = (EEProm.nodeID  & 0x3F) + 1;
  if ((dip1 == OFF) && (dip2 == ON)) nodeID = (EEProm.nodeID  & 0x3F) + 2;
  if ((dip1 == ON) && (dip2 == ON))  nodeID = (EEProm.nodeID  & 0x3F) + 3;
  
  Serial.print(F("Group ")); Serial.print(EEProm.networkGroup);
  Serial.print(F(", Node ")); Serial.print(nodeID);
  Serial.print(F(", Band ")); 
  Serial.print(EEProm.RF_freq == RF69_433MHZ ? 433 : 
               EEProm.RF_freq == RF69_868MHZ ? 868 :
               EEProm.RF_freq == RF69_915MHZ ? 915 : 0);
  Serial.println(F(" MHz"));
 
  Serial.print(F("pulses enabled = ")); Serial.println(EEProm.pulse_enable);
  Serial.print(F("pulse period = ")); Serial.print(EEProm.pulse_period); Serial.println(" ms");
  Serial.print(F("DS18B20 enabled = ")); Serial.println(EEProm.temperatureEnabled);
  Serial.println(EEProm.rf_on & 0x01 ? "RF on":"RF off");
  Serial.println(EEProm.rf_on & 0x02 ? "Serial on":"Serial off");
  Serial.print(F("RF power = "));Serial.print(EEProm.rfPower - 18);Serial.println(" dBm");
}

static void save_config()
{
  Serial.println("Saving...");
  eepromWrite(eepromSig, (byte *)&EEProm, sizeof(EEProm));
  eepromPrint();
  Serial.println();
  Serial.println(F("Done. New config saved to EEPROM"));
}

static void wipe_eeprom()
{
  Serial.println(F("Resetting..."));
  eepromHide(eepromSig);   
  Serial.println(F("emonTH restarting with default config."));
  delay(200);
}

void softReset(void)
{
  asm volatile ("  jmp 0");
}


int getPass(void)
{
/*
 * Get and verify the user's access code (replaces 'readString()' saves 1300 bytes)
 */
  char buf[5];
  if (Serial.readBytes(buf, 5) != 5)
    return 0;
  if  (buf[0] == '+' 
    && buf[1] == '+'
    && buf[3] == '\r'
    && buf[4] == '\n')
    {
      if (buf[2] == '+')      // Verbose
      {
        calibration_enable = true;
        return 2;
      }
      else if (buf[2] == 's') //Silent
      {
        calibration_enable = true;
        return 1;
      }
    }
  return 0;
}


void getSettings(void)
{
/*
 * Reads settings information (if available and permitted) from the serial port
 *  see the user instruction above, the comments below or the separate documentation for details
 *
 * Data is expected generally in the format
 * 
 *  [l] [x] [y] [z]
 * 
 * where:
 *  [l] = a single letter denoting the variable to adjust
 *  [x] [y] [z] etc are values to be set.
 * 
 */
  Serial.setTimeout(2000); //allow more time before serial timeout for multi-digit entry in parseInt() etc.
 
  Serial.println("'+++' then [Enter] for config mode, waiting 4s...");
  Serial.println(EEProm.rf_on & 0x02 ? "Serial on":"Serial print off");
  //Serial.println("(Arduino IDE Serial Monitor: make sure 'Both NL & CR' is selected)");
  start = millis();

  while (millis() < (start + 4000))
  {
    // If serial input of keyword string '+++' is entered during 5s power-up then enter config mode

    if (Serial.available() >= 5) //wait until all keyword string captured before checking
    {
      if (!calibration_enable) 
      {
        static char pass_result;
        if (pass_result = getPass())
        {
          Serial.println(F("Entering Settings mode..."));
          if (pass_result == 2) 
            showString(helpText);
        }
      }
    }
  }
  

  
  while (calibration_enable)
  {
    if (Serial.available())
    {
      char c = Serial.read();
      int k1;
      double k2;
      byte b;
	  
      switch (c) {
        case 'b':  // set band: 4 = 433, 8 = 868, 9 = 915
          b = bandToFreq(Serial.parseInt());
          if(b) //don't update if invalid band
          {
            EEProm.RF_freq = b;
          }
          Serial.print(EEProm.RF_freq == RF69_433MHZ ? 433 : 
                       EEProm.RF_freq == RF69_868MHZ ? 868 :
                       EEProm.RF_freq == RF69_915MHZ ? 915 : 0);
          Serial.println(F(" MHz"));
          break;

        case 'g':  // set network group
          EEProm.networkGroup = Serial.parseInt();
          Serial.print(F("Group ")); Serial.println(EEProm.networkGroup);
          break;
          
        /* case 'i' below */
          
        case 'l':
          list_calibration(); // print the settings & calibration values
          break;
            
        case 'm' :
          /*  Format expected: m[x] [y]
           * 
           * where:
           *  [x] = a single numeral: 0 = pulses OFF, 1 = pulses ON,
           *  [y] = an integer for the pulse min period in ms - ignored when x=0
           */
          k1 = Serial.parseInt(); 
          k2 = Serial.parseFloat(); 
          while (Serial.available())
            Serial.read(); 

          switch (k1) {
            case 0 : EEProm.pulse_enable = false;
              stopPulseCount();
              break;
            
            case 1 : EEProm.pulse_enable = true;
              EEProm.pulse_period = k2;
              startPulseCount();
              break;
          }
          Serial.print(F("Pulses: "));
          if (k1)
            {Serial.print(k2);Serial.println(F(" ms"));}
          else
            Serial.println("off");        
          break;        
    
        case 'i':  
        case 'n':  //  Set NodeID - range expected: 1 - 60
          EEProm.nodeID = Serial.parseInt();
          EEProm.nodeID = constrain(EEProm.nodeID, 1, 63);
          Serial.print(F("Node ")); Serial.println(EEProm.nodeID);
          break;
          
        case 'p': // set RF power level
          EEProm.rfPower = (Serial.parseInt() & 0x1F);
          Serial.print("p = ");Serial.print(EEProm.rfPower - 18);Serial.println(" dBm");
          break; 
          
        case 'r': // restore sketch defaults
          wipe_eeprom();
          softReset();
          break;

        case 's': // Save to EEPROM. ATMega328p has 1kB  EEPROM
          save_config();
          break;

        case 't' : // Temperatures
          /*  Format expected: t[x] [y] [y] ...
           */
          set_temperatures();
          break;

        case 'v': // print firmware version
          Serial.print(F("EmonTH_V2 V")); Serial.write(firmware_version);Serial.println("");
          break;
        
        case 'w' :  // Wireless / Serial - RF Off / On
          /* Format expected: w[x]
           */
          EEProm.rf_on = Serial.parseInt(); 
          Serial.println(EEProm.rf_on & 0x01 ? "RF on":"RF off");
          Serial.println(EEProm.rf_on & 0x02 ? "Serial on":"Serial off");
          break;
          
        case 'x':  // exit and continue
          Serial.println(F("Continuing..."));
          while (Serial.available())
            Serial.read(); 
          calibration_enable = false;
          delay(10);
          break;

        case '?':  // show Help text        
          showString(helpText);
          Serial.println(F(" "));
          break;
        
        default:
          ;
      } //end switch
      while (Serial.available())
        Serial.read(); 
    }
  }
}


static byte bandToFreq (byte band) {
  return band == 4 ? RF69_433MHZ : band == 8 ? RF69_868MHZ : band == 9 ? RF69_915MHZ : 0;
}


static void showString (PGM_P s) {
  for (;;) {
    char c = pgm_read_byte(s++);
    if (c == 0)
      break;
    if (c == '\n')
      Serial.print('\r');
    Serial.print(c);
  }
}

void set_temperatures(void)
{
  /*  Format expected: t[x] [y] [y] ...
  * 
  * where:
  *  [x] = 0  [y] = single numeral: 0 = temperature measurement OFF, 1 = temperature measurement ON
  *  [x] = a single numeral > 0: the position of the sensor in the list (1-based)
  *  [y] = 8 hexadecimal bytes representing the sensor's address
  *          e.g. t2 28 81 43 31 07 00 00 D9
  */
    
  DeviceAddress sensorAddress;
         
	unsigned int k1 = Serial.parseInt();

  if (k1 == 0)
     // write to EEPROM
    EEProm.temperatureEnabled = Serial.parseInt();
  else if (k1 > sizeof(EEProm.allAddresses) / sizeof(DeviceAddress))
    return;
  else
  {
    byte i = 0, a = 0, b;
    Serial.readBytes(&b,1);     // expect a leading space
    while (Serial.readBytes(&b,1) && i < 8)
    {            
      if (b == ' ' || b == '\r' || b == '\n')
      {
        sensorAddress[i++] = a;
        a = 0;
      }                
      else
      {
        a *= 16;
        a += c2h(b);
      }          
    }     
    // set address
    for (byte i=0; i<8; i++)
      EEProm.allAddresses[k1-1][i] = sensorAddress[i];
  }
}

byte c2h(byte b)
{
  if (b > 47 && b < 58) 
    return b - 48;
  else if (b > 64 && b < 71) 
    return b - 55;
  else if (b > 96 && b < 103) 
    return b - 87;
  return 0;
}


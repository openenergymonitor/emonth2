#include <RFM69.h>
RFM69 radio;

byte nativeMsg[66];

#define MAXMSG 66
char outmsg[MAXMSG];
byte outmsgLength;

struct {
  byte srcNode = 0;
  byte msgLength = 0;
  signed char rssi = -127;
  bool crc = false;
} rfInfo;

void setup() 
{
  Serial.begin(38400);
  //Serial.println("Repeater");
  radio.initialize(RF69_433MHZ,5,200);  
  radio.encrypt("89txbe4p8aik5kt3");
  radio.setPowerLevel(31);
}

void loop()             
{
  if (radio.receiveDone())
  {
    rfInfo.srcNode = radio.SENDERID;
    rfInfo.msgLength = radio.DATALEN;
    for (byte i = 0; i < radio.DATALEN; i++) {
      nativeMsg[i] = radio.DATA[i];
    }
    rfInfo.rssi = radio.readRSSI();
    
    if (radio.ACKRequested()) {
      radio.sendACK();
    }
    
    Serial.print(F("OK")); 
    Serial.print(F(" "));
    Serial.print(rfInfo.srcNode, DEC);
    Serial.print(F(" "));
    for (byte i = 0; i < rfInfo.msgLength; i++) {
      Serial.print((word)nativeMsg[i]);
      Serial.print(F(" "));
    }
    Serial.print(F("("));
    Serial.print(rfInfo.rssi);
    Serial.print(F(")"));
    Serial.println();
    
    // Repeat
    radio.setAddress(rfInfo.srcNode);
    radio.setNetwork(210);
    radio.sendWithRetry(5, nativeMsg, rfInfo.msgLength);
    radio.setAddress(5);
    radio.setNetwork(200);
  }
}
#define RF69_COMPAT 1
#include <JeeLib.h>

void setup() 
{
  Serial.begin(115200);
  Serial.println("Starting");
  rf12_initialize(6, RF12_433MHZ, 2);
}

void loop()
{
  if (rf12_recvDone())
  {
    if (rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0)  // and no RF errors
    {
      int node_id = (rf12_hdr & 0x1F);
      Serial.println(node_id);

      delay(100);

      // Forward the packet to the other radio group
      rf12_initialize(node_id, RF12_433MHZ, 1);
      rf12_sendStart(0, rf12_data, rf12_len);
      rf12_sendWait(0);
      rf12_initialize(6, RF12_433MHZ, 2); // Switch back to receiver
      
    }
  }
}

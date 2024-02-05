#include <Wire.h>

#define CHIP_ADDRESS 0x50

void setup() 
{
  Serial.begin(9600);
  Serial.println("EDID");
  Wire.begin();
}

void loop() {
  int i,j;
  
  Wire.beginTransmission(CHIP_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();

  for (j=0; j<16; j++)
  {
    Wire.requestFrom(CHIP_ADDRESS, 16, true);
    for (i=0; i<16; i++)
    {
      uint8_t val = Wire.read();
      Serial.print(val,HEX);
      Serial.print(" ");
    }
    Serial.println();
  }

  Serial.println("DONE"); 
  for (;;) { delay(100); }
}

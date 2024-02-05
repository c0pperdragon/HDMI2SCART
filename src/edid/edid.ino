// test program to deliver the necessary EDID information
// uses a regular Arduino Uno

// EDID data block
uint8_t edid[128] = {
0x0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x0,0x10,0xAC,0x41,0x40,0x55,0x35,0x39,0x32, 
0x1C,0x12,0x1,0x3,0xE,0x2B,0x1B,0x78,0x2E,0xB7,0xF5,0xA8,0x54,0x35,0xAD,0x25, 
0x10,0x50,0x54,0xBF,0xEF,0x80,0x71,0x4F,0x81,0x80,0xB3,0x0,0x95,0x0,0x81,0xC0, 
0x81,0x0,0x1,0x1,0x1,0x1,0x21,0x39,0x90,0x30,0x62,0x1A,0x27,0x40,0x68,0xB0, 
0x36,0x0,0xB1,0xE,0x11,0x0,0x0,0x1C,0x0,0x0,0x0,0xFF,0x0,0x47,0x34,0x34, 
0x39,0x48,0x38,0x37,0x36,0x32,0x39,0x35,0x55,0xA,0x0,0x0,0x0,0xFC,0x0,0x44, 
0x45,0x4C,0x4C,0x20,0x32,0x30,0x30,0x39,0x57,0xA,0x20,0x20,0x0,0x0,0x0,0xFD, 
0x0,0x38,0x4B,0x1E,0x53,0x10,0x0,0xA,0x20,0x20,0x20,0x20,0x20,0x20,0x0,0xCA 

/*
  0,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0,    // 0-7 fixed header
  0x0D, 0xF0,                           // 8-9 manufacturer: "COP"
  0x01, 0x00,                           // 10-11 product code: 1
  0x00, 0x00, 0x00, 0x00,               // 12-15 serial number: 0
  10, 34,                               // 16-17 week and year (10,2024)
  1, 4,                                 // 18-19 EDID version
  B00000101,                            // 20 video parameters: analog with csync
  40,                                   // 21 screen width in centimeters
  30,                                   // 22 screen height in centimeters
  0,                                    // 23 display gamma value
  B00001010,                            // 24 feature bitmap (RGB color)
  0,0,0,0,0,0,0,0,0,0,                  // 25-34 chromaticity coordinates
  0x00, 0x00, 0x00,                     // 35-37 established timings bitmap
  0x01, 0x01,                           // 38-39 standard timing 1 (unused)
  0x01, 0x01,                           // 40-41 standard timing 2 (unused)
  0x01, 0x01,                           // 42-43 standard timing 3 (unused)
  0x01, 0x01,                           // 44-45 standard timing 4 (unused)
  0x01, 0x01,                           // 46-47 standard timing 5 (unused)
  0x01, 0x01,                           // 48-49 standard timing 6 (unused)
  0x01, 0x01,                           // 50-51 standard timing 7 (unused)
  0x01, 0x01,                           // 52-53 standard timing 8 (unused)
                                        // 54-71 detailed timing 1 
  3000%256, 3000/256,                   //   0-1 pixel clock: 30MHz
  1584%256,                             //   2 horizontal active pixels 8 lsb    
  336%256,                              //   3 horizontal blanking pixels 8 lsb    
  ((1584/256)<<4) + (336/256),          //   4 hor act & blanking 4 msb each
  288%256,                              //   5 vertical active lines 8 lsb
  24%256,                               //   6 vertical blanking lines 8 lsb
  ((288/256)<<4) + (24/256),            //   7 ver act & blanking 4 msb each
  48%256,                               //   8 horizontal front porch 8 lsb   
  144%256,                              //   9 horizontal sync pulse width 8 lsb
  ((2%16)<<4) + (2%16),                 //   10 vert front porch & sync 4 lsb each
  ((48/256)<<6) + ((144/256)<<4)        //   11 hor fp, hor sync, 
  + ((2/16)<<2) + (2/16),               //      vert fp, vert sync, 2 msb each
  400%256,                              //   12 horizontal image size (mm), 8 lsb
  300%256,                              //   13 vertical image size (mm), 8 lsb
  ((400/256)<<4) + (300/256),           //   14 hor & vert image size 4msb each
  0,                                    //   15 horizontal border
  0,                                    //   16 vertical border
  B00000000,                            //   17 features bitmap
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  // 72-89 detailed timing 2 
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  // 90-107 detailed timing 3 
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  // 108-125 detailed timing 4 
  0,                                    // 126 number of extensions
  0                                     // 127 placeholder for checksum 
*/
};

#include <Wire.h>


uint8_t registeraddress = 0;

void setup() {
  uint8_t i;
  uint8_t sum;
  // compute EDID checksum
  sum = 0;
  for (i=0; i<127; i++) { sum = sum + edid[i]; }
  edid[127] = 256 - sum;

//  Serial.begin(9600);
//  Serial.print("CHECKSUM ");
//  Serial.print(edid[127], HEX);
//  Serial.println();
  
  // start to listen on I2C interface
  Wire.begin(0x50);             // join i2c bus with address #$50
  Wire.onRequest(requestEvent); // register event
  Wire.onReceive(receiveEvent);
}

void loop() 
{
}

void receiveEvent(int numbytes)
{
  if (numbytes>0) { 
    registeraddress = Wire.read(); 
  }
  while (Wire.available()>0) { Wire.read(); }
}

void requestEvent() 
{
  Wire.write(edid[registeraddress]);
  registeraddress = (registeraddress+1) & 0x7F;   
}

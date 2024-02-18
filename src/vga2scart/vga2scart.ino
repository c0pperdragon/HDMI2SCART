// Create a bridge between a VGA output and a SCART input.
// As the analog RGB channels are basially identical, only some 
// control signals need to be provided or translated.
// The intemediary microcontroller has the following tasks:
// 1. Deliver a EDID to the host signalling a 288p/240p display.
// 2. Synthesize a CSYNC signal from HSYNC/VSYNC
//
// Uses an Attiny1604, with standard I2C wiring.
// Set to turn off millis() timer and clock speed must be 16MHz 

void setup() 
{
  CLKCTRL.MCLKCTRLB = B00000000; // use full system clock for peripherials
  pinMode(8, INPUT);   // pin 11/PA1 (HSYNC)
  pinMode(9, INPUT);   // pin 12/PA2 (VSYNC)
  pinMode(0, OUTPUT);  // pin 2/PA4 (CSYNC)
  digitalWrite(0,LOW);

  pinMode(7,INPUT_PULLUP); // pin 9/PB0 (SCL)
  pinMode(6,INPUT_PULLUP); // pin 8/PB1 (SDA)
  
  startcsync();
  calculate_checksums();    
  starti2c();
}

void loop()
{
}

// set up programmable logic to make simple XNOR combination for CSYNC
void startcsync()
{
  // combine signals in asynchronous LUT
  CCL.LUT0CTRLB   = 0x50;       // input 1 from PA1, input 0 unused  
  CCL.LUT0CTRLC   = 0x05;       // input 2 from PA2
  CCL.TRUTH0      = B11000011;  // truth table for XNOR of input 1 and 2 
  CCL.LUT0CTRLA   = B00001001;  // enable LUT0 and dedicated output pin
  CCL.CTRLA = B00000001;        // enable CCL  
}


// EDID data block to emulate a VGA screen
uint8_t edid50hz[128] = {
  0x00,0xff,0xff,0xff,0xff,0xff,0xff,0x00,                                                    // standard header
  0x0d,0xf0,0x01,0x00,0x01,0x00,0x00,0x00,0x0a,0x22,                                          // manufacturer info
  0x01,0x03,                                                                                  // EDID version
  B00001000,                                                                                  // video input parameters
  0x28,0x1e,                                                                                  // screen size in cm
  0x78,                                                                                       // gamma factor
  B00001110,                                                                                  // supported features bitmap    
  0xEE,0x91,0xA3,0x54,0x4C,0x99,0x26,0x0F,0x50,0x54,                                          // chromaticity coordinates 
  0x00,0x00,0x00,                                                                             // established modes 
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,            // standard modes
  0x88,0x0a,0xa0,0x20,0x51,0x20,0x18,0x10,0x18,0x80,0x33,0x00,0x90,0x2c,0x11,0x00,0x00,0x18,  // detailed timing descriptor
  0x00,0x00,0x00,0xfc,0x00,0x48,0x44,0x4d,0x49,0x32,0x53,0x43,0x41,0x52,0x54,0x0a,0x20,0x20,  // monitor information
  0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // dummy
  0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // dummy 
  0x00,                                                                                       // number of extension blocks
  0x00,                                                                                       // place for checksum 
};
uint8_t edid60hz[128] = {
  0x00,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x0d,0xf0,0x01,0x00,0x01,0x00,0x00,0x00,
  0x0a,0x22,0x01,0x03,0xa2,0x28,0x1e,0x78,0x26,0xEE,0x91,0xA3,0x54,0x4C,0x99,0x26,
  0x0F,0x50,0x54,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
  0x01,0x01,0x01,0x01,0x01,0x01,0x88,0x0a,0xa0,0x20,0x51,0xf0,0x16,0x00,0x18,0x80,
  0x33,0x00,0x90,0x2c,0x11,0x00,0x00,0x18,0x00,0x00,0x00,0xfc,0x00,0x48,0x44,0x4d,
  0x49,0x32,0x53,0x43,0x41,0x52,0x54,0x0a,0x20,0x20,0x00,0x00,0x00,0x10,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,
};


void calculate_checksums()
{
  uint8_t cs1 = 0;
  uint8_t cs2 = 0;
  uint8_t i;
  for (i=0; i<127; i++)
  {
      cs1 += edid50hz[i];
      cs2 += edid60hz[i];
  }
  edid50hz[127] = 0 - cs1;
  edid60hz[127] = 0 - cs2;
}

uint8_t registeraddress = 0;
bool didreceiveaddress = false;

void starti2c()
{
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  TWI0.CTRLA       = B00011100;  // most conservative behaviour
  TWI0.SCTRLB      = B00000000;  // standard ACK behaviour
  TWI0.SADDR       = B10100000;  // client address $50, no response to "general call address"
  TWI0.SCTRLA      = B11100011;  // enable TWI client in smart mode and allow interrupts  
}

ISR(TWI0_TWIS_vect)
{
  switch (TWI0.SSTATUS & B11000011)
  {
  case B01000001:     // address interrupt for writing
    didreceiveaddress = false;
    break;
  case B10000000:     // data interrupt when writing
  case B10000001:
    if (! didreceiveaddress) 
    {
      registeraddress = TWI0.SDATA;   // already does ACK
      didreceiveaddress = true;
      return;
    }
    break;
  case B10000010:     // data interrupt when reading
  case B10000011:
    // check if using 50Hz mode
    if (true) { TWI0.SDATA = edid50hz[registeraddress & 0x7f]; }  // already does ACK
    else      { TWI0.SDATA = edid60hz[registeraddress & 0x7f]; }  // already does ACK
    registeraddress++;
    return;
  }
  TWI0.SCTRLB = 0x03;  // acknowledge every other case
}
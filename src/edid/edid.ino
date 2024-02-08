// Deliver a customized EDID to the host computer.
// Uses an Attiny1404, with standard I2C wiring.
// Set to turn off millis() timer and set 16MHz 

void setup() 
{
  CLKCTRL.MCLKCTRLB = B00000000; // use full system clock for peripherials
  pinMode(8, INPUT); // pin A1 (HSYNC) 
  pinMode(9, INPUT); // pin A2 (VSYNC)
  pinMode(0, OUTPUT);  // CSYNC
  digitalWrite(0,LOW);
    
  startcsync();
  starti2c();
}

void loop() 
{
  for (;;)
  {
    // wait for falling edge on HSYNC (A1)
    while (PORTA.IN & B00000010);
    CCL.CTRLA = B00000000;   // disable CCL to modify the LUT definition
    CCL.LUT0CTRLA = B00000000;  // disable LUT0 and dedicated output pin
    // when not in VSYNC (A2), revert to straight HSYNC use
    if (PORTA.IN & B00000100)
    {
      CCL.TRUTH0 = B10101010;
      CCL.LUT0CTRLA = B00001001;  
      CCL.CTRLA = B00000001;
      while (!(PORTA.IN & B00000010));  // after change wait for rising HSYNC
    }
    // when in VSYNC combine HSYNC and extended HSYNC
    else
    {
      while (!(PORTA.IN & B00000010)); // do change after rising HSYNC
      CCL.TRUTH0 = B00100010;
      CCL.LUT0CTRLA = B00001001;  
      CCL.CTRLA = B00000001;      
    }
  }
}

void startcsync()
{
  // set up async channel 0 to feed the hsync input pin into timer B and to LUT0
  EVSYS.ASYNCCH0 = 0x0B;   // HSYNC from pin A1
  EVSYS.ASYNCUSER0 = 0x03; // timer B uses async channel 0 as input
  EVSYS.ASYNCUSER2 = 0x03; // LUT0 uses async channel 0 as event source 0
  // configure timer B to generate a longer pulse for each incomming HSYNC
  TCB0.CCMP = 820;         // duration of single-shot pulse  
  TCB0.CTRLA = B00000001;  // enable timer B at full speed
  TCB0.CTRLB = 0x06;       // single-shot mode
  TCB0.EVCTRL = B01000001;  // start at positive edge
  // combine signals in asynchronous LUT
  CCL.LUT0CTRLB   = 0x73;       // input 1 from from timer B and HSYNC via its event source 0  
  CCL.LUT0CTRLC   = 0x00;       // don't usee
  CCL.TRUTH0      = B10101010;  // truth table to take HSYNC directly 
  CCL.LUT0CTRLA   = B00001001;  // enable LUT0 and dedicated output pin
  CCL.CTRLA = B00000001;        // enable CCL  
}


// EDID data block
uint8_t edid[128] = {
  0x00,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x0d,0xf0,0x01,0x00,0x01,0x00,0x00,0x00,
  0x0a,0x22,0x01,0x03,0xa2,0x28,0x1e,0x78,0x26,0xb7,0xf5,0xa8,0x54,0x35,0xad,0x25,
  0x10,0x50,0x54,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
  0x01,0x01,0x01,0x01,0x01,0x01,0x88,0x0a,0xa0,0x20,0x51,0x20,0x18,0x10,0x18,0x80,
  0x33,0x00,0x90,0x2c,0x11,0x00,0x00,0x18,0x00,0x00,0x00,0xfc,0x00,0x48,0x44,0x4d,
  0x49,0x32,0x53,0x43,0x41,0x52,0x54,0x0a,0x20,0x20,0x00,0x00,0x00,0x10,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0c
};

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
    TWI0.SDATA = edid[registeraddress & 0x7F];   // allready does ACK
    registeraddress++;
    return;
  }
  TWI0.SCTRLB = 0x03;  // acknowledge every case
}

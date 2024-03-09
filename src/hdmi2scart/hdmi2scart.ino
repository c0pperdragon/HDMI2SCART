
// Provide additional functionality to make the HDMItoVGA adapter generate a SCART output signal.
// There are three basic task, that the microcontroller has to do:
// 1. Deliver a EDID to the host signalling a 288p/240p display.
// 2. Synthesize a CSYNC signal from HSYNC/VSYNC
// 3. Turn hot-plug detection off/on on change of mode selector
//
// Uses an Attiny1604, with standard I2C wiring, 20Mhz

void setup() 
{
  CLKCTRL.MCLKCTRLB = B00000000; // use full system clock for peripherials
  pinMode(8, INPUT);        // pin 11/PA1 (VSYNC)
  pinMode(9, INPUT);        // pin 12/PA2 (HSYNC)
  pinMode(0, OUTPUT);       // pin 2/PA4 (CSYNC)
  digitalWrite(0,HIGH);

  pinMode(7, INPUT_PULLUP); // pin 9/PB0 (SCL)
  pinMode(6, INPUT_PULLUP); // pin 8/PB1 (SDA)

  pinMode(3, INPUT_PULLUP); // pin 5/PA7 (60Hz selector)
  pinMode(5, OUTPUT);       // pin 7/PB2 (hot-plug detection)
  digitalWrite(5,HIGH);
  
  startcsync();
  calculate_checksums();    
  starti2c();
}

// continue monitoring the hsync and vsync inputs to measure their polarity
void loop()
{
  bool hsync_pos = false;
  bool vsync_pos = false;
  bool h;
  bool v;
  byte selector = digitalRead(3);
  for (;;)
  {
    // sample vsync and hsync many times to decide polarity
    unsigned int i,j;
    unsigned int h_highcounter=0;
    unsigned int h_lowcounter=0;
    unsigned int v_highcounter=0;
    unsigned int v_lowcounter=0;
    for (i=0; i<1500; i++)
    {
      if (digitalRead(9)==LOW) { h_lowcounter++; }
      else                     { h_highcounter++; }
      if (digitalRead(8)==LOW) { v_lowcounter++; }
      else                     { v_highcounter++; }
    }    
    // when some polarity is different than what is set, change input setting
    h = (h_highcounter < h_lowcounter);
    v = (v_highcounter < v_lowcounter);
    if ((h!=hsync_pos) || (v!=vsync_pos)) {
      setinputpolarity(h,v);
      hsync_pos = h;
      vsync_pos = v;
    }
    // test for change of selector switch and emmit low level on device detection pin
    if (digitalRead(3) != selector) 
    {
      digitalWrite(5,LOW);
      for (j=0; j<30; j++) for (i=0; i<10000; i++) { digitalRead(3); }  // for 0.5 s delay
      digitalWrite(5,HIGH);
      selector = digitalRead(3);
    }
  }
}

// set up programmable logic to synthesize csync from VSYNC and HSYNC (negative polarity by default)
void startcsync()
{
  // configure LUT1 to invert incomming HSYNC from PA2 and provide signal on async channel 1
  EVSYS.ASYNCCH0 = 0x0C;        // async channel 0 use data from PA2
  EVSYS.ASYNCUSER3 = 0x3;       // EV0 input for LUT1 is taken from async channel 0
  CCL.LUT1CTRLB   = 0x03;       // input 1 unused, input 0 from EV0 input  
  CCL.LUT1CTRLC   = 0x00;       // input 2 unused
  CCL.TRUTH1      = B01010101;  // truth table for negation of input 0 
  CCL.LUT1CTRLA   = B00000001;  // enable LUT1 (but not output pin)
  EVSYS.ASYNCCH1 = 0x02;        // async channel 1 use data from LUT1 
  // set up timer B in single-shot mode to make an extended hsync signal for use during vsync if needed
  EVSYS.ASYNCUSER0 = 0x04;  // timer B is event user of async channel 1 
  TCB0.CTRLA = 0x01;        // enable timer B, using full peripherial clock
  TCB0.CTRLB = 0x06;        // single-shot mode, nothing fancy extra
  TCB0.EVCTRL = 0x01;       // capture positive edge 
  TCB0.CCMP = 1100;         // nominally 55us duration of single-shot pulse 
  // finally combine signals in asynchronous LUT to use the one-shot pulse during vsync, otherwise the hsync
  EVSYS.ASYNCUSER2 = 0x04;      // EV0 input for LUT0 is taken from async channel 1
  CCL.LUT0CTRLB   = 0x57;       // input 1 from PA1, input 0 from TCB state
  CCL.LUT0CTRLC   = 0x03;       // input 2 from EV0 input which is async channel 1 (from LUT1)
  CCL.TRUTH0      = B00001101;  // truth table for using negative vsync input
  CCL.LUT0CTRLA   = B00001001;  // enable LUT0 and dedicated output pin
  // turn on CCL
  CCL.CTRLA = B00000001;        // enable CCL in general
}

// change input polarity 
void setinputpolarity(bool hsync_pos, bool vsync_pos)
{
  // turn off CCL to allow modification
  CCL.CTRLA = B00000000;       

  CCL.LUT1CTRLA   = B00000000;  // disable LUT1 to allow modification
  CCL.TRUTH1 = hsync_pos ? B10101010 : B01010101;   // optional negation of hsync input 
  CCL.LUT1CTRLA   = B00000001;  //  enable LUT1 again

  CCL.LUT0CTRLA   = B00000000;  // disable LUT0 to allow modification
  CCL.TRUTH0 = vsync_pos ? B00000111 : B00001101;   // two possible tables 
  CCL.LUT0CTRLA   = B00001001;  //  enable LUT0 again (with output to pin)

  // turn on CCL
  CCL.CTRLA = B00000001;       
}


// EDID data block to emulate an HDMI screen
uint8_t edid50hz[256] = {
  0x00,0xff,0xff,0xff,0xff,0xff,0xff,0x00,                                                    // standard header
  0x0d,0xf0,0x01,0x00,0x01,0x00,0x00,0x00,0x0a,0x22,                                          // manufacturer info
  0x01,0x03,                                                                                  // EDID version
  0xa0,                                                                                       // video input parameters
  0x28,0x1e,                                                                                  // screen size in cm
  0x78,                                                                                       // gamma factor
  0x06,                                                                                       // supported features bitmap    
  0xEE,0x91,0xA3,0x54,0x4C,0x99,0x26,0x0F,0x50,0x54,                                          // chromaticity coordinates 
  0x00,0x00,0x00,                                                                             // established modes
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,            // standard modes
  0x8c,0x0a,0xa0,0x20,0x51,0x20,0x18,0x10,0x18,0x80,0x33,0x00,0x90,0x2c,0x11,0x00,0x00,0x18,  // detailed timing descriptor: 1440x288
  0x00,0x00,0x00,0xfc,0x00,0x48,0x44,0x4d,0x49,0x32,0x53,0x43,0x41,0x52,0x54,0x0a,0x20,0x20,  // monitor information
  0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // dummy
  0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // dummy
  0x01,                                                                                       // number of extension blocks
  0x00,                                                                                       // place for checksum

  0x02,0x03,0x10,0x40,                                                                        // header: support basic audio, 0 native formats
  0x23,0x09,0x07,0x07,                                                                        // audio data block
  0x83,0x01,0x20,0x20,                                                                        // speaker allocation
  0x63,0x03,0x0c,0x00,                                                                        // IEEE registration identifier
  0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00  
};
uint8_t edid60hz[256] = {
  0x00,0xff,0xff,0xff,0xff,0xff,0xff,0x00,                                                    // standard header
  0x0d,0xf0,0x01,0x00,0x01,0x00,0x00,0x00,0x0a,0x22,                                          // manufacturer info
  0x01,0x03,                                                                                  // EDID version
  0xa0,                                                                                       // video input parameters
  0x28,0x1e,                                                                                  // screen size in cm
  0x78,                                                                                       // gamma factor
  0x06,                                                                                       // supported features bitmap    
  0xEE,0x91,0xA3,0x54,0x4C,0x99,0x26,0x0F,0x50,0x54,                                          // chromaticity coordinates 
  0x00,0x00,0x00,                                                                             // established modes
  0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,            // standard modes
  0x8c,0x0a,0xa0,0x14,0x51,0xf0,0x16,0x00,0x18,0x80,0x33,0x00,0x90,0x2c,0x11,0x00,0x00,0x18,  // detailed timing descriptor: 1440x240
  0x00,0x00,0x00,0xfc,0x00,0x48,0x44,0x4d,0x49,0x32,0x53,0x43,0x41,0x52,0x54,0x0a,0x20,0x20,  // monitor information
  0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // dummy
  0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // dummy
  0x01,                                                                                       // number of extension blocks
  0x00,                                                                                       // place for checksum

  0x02,0x03,0x10,0x40,                                                                        // header: support basic audio, 0 native formats
  0x23,0x09,0x07,0x07,                                                                        // audio data block
  0x83,0x01,0x20,0x20,                                                                        // speaker allocation
  0x63,0x03,0x0c,0x00,                                                                        // IEEE registration identifier
  0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00  
};

void calculate_checksums()
{
  uint8_t cs1 = 0;
  uint8_t cs2 = 0;
  uint8_t cs3 = 0;
  uint8_t cs4 = 0;
  uint8_t i;
  for (i=0; i<127; i++)
  {
      cs1 += edid50hz[i];
      cs2 += edid50hz[128+i];
      cs3 += edid60hz[i];
      cs4 += edid60hz[128+i];
  }
  edid50hz[127] = 0 - cs1;
  edid50hz[255] = 0 - cs2;
  edid60hz[127] = 0 - cs3;
  edid60hz[255] = 0 - cs4;
}

uint8_t registeraddress = 0;
bool didreceiveaddress = false;

void starti2c()
{
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
    if (PORTA.IN & B10000000) { TWI0.SDATA = edid50hz[registeraddress]; }  // already does ACK
    else                      { TWI0.SDATA = edid60hz[registeraddress]; }  // already does ACK
    registeraddress++;
    return;
  }
  TWI0.SCTRLB = 0x03;  // acknowledge every other case
}

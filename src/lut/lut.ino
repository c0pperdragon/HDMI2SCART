// Test both LUTs of the Attiny1604.
// LUT0:
//  input A: unused
//  input B: pin PA1(11/Arduino 8)
//  input C: pin PA2(12/Arduino 9)
//  output:  pin PA4(2/Arduino 0)   = B and C
// LUT1:
//  input A: unused
//  input B: pin PA6(4/Arduino 2)
//  input C: pin PB3(6/Arduino 4)
//  output:  pin PA7(5/Arduino 3)  = B or C


void setup() 
{
  pinMode(8,INPUT_PULLUP);
  pinMode(9,INPUT_PULLUP);
  pinMode(0,OUTPUT);
  pinMode(2,INPUT_PULLUP);
  pinMode(4,INPUT_PULLUP);
  pinMode(3,OUTPUT);
  
  // set up async channels 0/1 to bring inputs to LUT1
  EVSYS.ASYNCCH0 = 0x10 ;   // from pin PA6
  EVSYS.ASYNCUSER3 = 0x03;  // LUT1 uses this as event source 0
  EVSYS.ASYNCCH1 = 0x0D ;   // from pin PB3
  EVSYS.ASYNCUSER5 = 0x04;  // LUT1 uses this as event source 1
  // configure LUT0 as AND gate
  CCL.LUT0CTRLB   = 0x50;       // input 1 from dedicated pin, don't use input 0  
  CCL.LUT0CTRLC   = 0x05;       // input 2 from dedicated pin
  CCL.TRUTH0      = B11000000;  // truth table for AND 
  CCL.LUT0CTRLA   = B00001001;  // enable LUT0 and dedicated output pin
  // configure LUT1 as OR gate
  CCL.LUT1CTRLB   = 0x30;       // input 1 from event source 0, don't use input 0  
  CCL.LUT1CTRLC   = 0x04;       // input 2 from event source 1
  CCL.TRUTH1      = B11111100;  // truth table for OR 
  CCL.LUT1CTRLA   = B00001001;  // enable LUT1 and dedicated output pin
  // enable/lock LUTs in general
  CCL.CTRLA = B00000001;        // enable CCL  
}

void loop() 
{
}


#define BUFFERSIZE 100

byte buffer[BUFFERSIZE];
  int bufferfill=0;
  byte prev;
  byte clk_dta=0;
  boolean started=false;
  byte bitsread=0;
  byte bitbuffer=0;

void setup() 
{  
  pinMode(A5,INPUT_PULLUP);
  pinMode(A4,INPUT_PULLUP);

  Serial.begin(9600);
  Serial.write("CAPTURE RUNNING\n");
  delay(100);
  
  noInterrupts();
  capture();
  interrupts();
  
  Serial.write("DUMP\n");
  Serial.write(buffer, BUFFERSIZE);
}

void loop() 
{
}

void capture()
{
  
  while (bufferfill<BUFFERSIZE)
  {
    prev = clk_dta;
    clk_dta = PINC;
    clk_dta = (clk_dta>>4) & 0x03;
    
    if ( (prev ^ clk_dta) == B00000011) // both lines change simultaneously: error
    {
      buffer[bufferfill++] = 'X';
      started = false;
    }
    else if ((prev==B00000011) && (clk_dta==B00000010)) // start signal
    {
      started = true;
      bitsread = 0;
      buffer[bufferfill++] = '>';
    }
    else if ((prev==B00000010) && (clk_dta==B00000011)) // stop signal
    {
      started=false;
      buffer[bufferfill++] = '\n';
    }
/*    
    else if (started && ((prev&B00000010)!=0) && ((clk_dta&B00000010)==0))  // low going clock
    {
      bitbuffer = (bitbuffer<<1) | (clk_dta&1);
      bitsread++;
      if ((bitsread==5) || (bitsread==9))  // including non-used first flank
      {
        byte d = (bitbuffer & 0x0f);
        if (d<10) {  buffer[bufferfill++] = d+'0'; }
        else { buffer[bufferfill++] = d+('A'-10); }
      }
      else if (bitsread==10) // also including the ack
      {
        if ((bitbuffer&0x01) == 0) { buffer[bufferfill++] = '-'; }
        else { buffer[bufferfill++] = '+'; }
        bitsread=0;
      }
    }
*/
  }
}

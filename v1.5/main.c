#include <main.h>

#use rs232(baud = 9600, xmit = PIN_C6, rcv = PIN_C7)

#bit SSPEN = 0x14.5
#bit SSPOV = 0x14.6
#bit WCOL  = 0x14.7

#DEFINE MAXSTR 64
#define TARGET_ADDRESS  0xB0    // slave address

#define SUCCESS      1
#define NOT_SUCCESS  0

#define CMD_SEND_BYTE_TO_SLAVE         1
#define CMD_READ_TWO_BYTES_FROM_SLAVE  2

char  BufferString[MAXSTR]={0};
int   BufferStringIndex = 0;
int   DataReady = 0;
////////////////////////////////////////////////////////////////////////////////  
///
///   M A I N    F U N C T I O N .
///
////////////////////////////////////////////////////////////////////// --- Main.
void ssp_interrupt();



////////////////////////////////////////////////////////
// function send LETTER (MANY CHARLECTOR) to XBEE.

void send_letter_to_XBEE(char* text){

   int i;
   i2c_start();
   i2c_write(XBEE_ADDRESS);
   i2c_write(SEND_CHAR);
   
   for(i=0;text[i]!='\0';i++)
   {
      i2c_write(text[i]);   
   }  
   i2c_write('\0');
   i2c_stop();
}

void resetI2C() {

   // clear the error flag registers and re-enable the i2c bus
   SSPEN = 0;   // disable i2c
   SSPOV = 0;   // clear the receive overflow flag
   WCOL = 0;    // clear the write collision flag

   SSPEN = 1;   // re-enable i2c   

}


///////////////////////////////////////////////
// function send charlector to XBEE.

void send_char_to_XBEE(char text){

   i2c_start();
   i2c_write(XBEE_ADDRESS);
   i2c_write(SEND_CHAR);
   i2c_write(text);
   i2c_write('\0');
   i2c_stop();
}

typedef enum {NIL, READY_FOR_ADDRESS, READY_FOR_CMD, READY_FOR_LETTER } State;
State mstate = NIL;

#INT_SSP
void ssp_interrupt()
{
   int state;
   int inByte;


   // Output of i2c_isr_state()
   //
   // 0         - Address match received with R/W bit clear, perform i2c_read( )
   //             to read the I2C address.
   // 1-0x7F    - Master has written data; i2c_read() will immediately return the data
   // 0x80      - Address match received with R/W bit set; perform i2c_read( ) to read
   //             the I2C address, and use i2c_write( ) to pre-load the transmit buffer
   //             for the next transaction (next I2C read performed by master will read
   //             this byte).
   // 0x81-0xFF - Transmission completed and acknowledged; respond with i2c_write() to
   //             pre-load the transmit buffer for the next transation (the next I2C
   //             read performed by master will read this byte).

   state = i2c_isr_state();
   printf(usb_cdc_putc, "i2c_isr_state %x\n", state);
   //////////////////////////////////////////////////////////////////
   //  Address match with bit 0 clear (master write to slave)
   //////////////////////////////////////////////////////////////////
   if (state == 0) {
      i2c_read();  // flush the device address in the rx buffer
      

      // Fix any errors in the I2C communication
      // if wrong state -> there must have been an error in the i2c comm
      if (mstate != READY_FOR_ADDRESS) {
         // clear the error flag registers and re-enable the i2c bus
         //SSPEN = 0;   // disable i2c
         //SSPOV = 0;   // clear the receive overflow flag
         //WCOL = 0;    // clear the write collision flag
         //SSPEN = 1;   // re-enable i2c

         mstate = READY_FOR_ADDRESS; // reset the state
         return;

      }
   
      mstate = READY_FOR_CMD;     //device moves into command mode
   }

   //////////////////////////////////////////////////////////////////
   //  a byte has been recieved from the Master
   //////////////////////////////////////////////////////////////////

   else if (state < 0x80) {
      inByte=i2c_read();

      switch (mstate) {        
      
        case READY_FOR_CMD:
      
            switch (inByte) {
               case 0x3:
                  mstate=READY_FOR_LETTER;        //first step of text display routine; can also display numbers
                  break;
               default:
                  mstate = READY_FOR_ADDRESS;
                  break;
            }
   
            break;
      
         case READY_FOR_LETTER:
            if(inByte != '\0')
            {
               printf(usb_cdc_putc, "READ %c\n", inByte);
               BufferString[BufferStringIndex] = inByte;
               BufferStringIndex++;
               BufferStringIndex %= MAXSTR;
                              
               mstate=READY_FOR_LETTER;
            }
            else
            {
               DataReady = 1;
               BufferStringIndex = 0;
               mstate=READY_FOR_ADDRESS;
            }
            break;                     
      }         
   } 
   else 
   {
      mstate = READY_FOR_ADDRESS;
   }
}

BYTE bin2bcd(BYTE binary_value) 
{ 
  BYTE temp; 
  BYTE retval; 

  temp = binary_value; 
  retval = 0; 

  while(1) 
  { 
    // Get the tens digit by doing multiple subtraction 
    // of 10 from the binary value. 
    if(temp >= 10) 
    { 
      temp -= 10; 
      retval += 0x10; 
    } 
    else // Get the ones digit by adding the remainder. 
    { 
      retval += temp; 
      break; 
    } 
  } 

  return(retval); 
} 

// Input range - 00 to 99. 
BYTE bcd2bin(BYTE bcd_value) 
{ 
  BYTE temp; 

  temp = bcd_value; 
  // Shifting upper digit right by 1 is same as multiplying by 8. 
  temp >>= 1; 
  // Isolate the bits for the upper digit. 
  temp &= 0x78; 

  // Now return: (Tens * 8) + (Tens * 2) + Ones 

  return(temp + (temp >> 2) + (bcd_value & 0x0f)); 
} 

int initDS1307() {

   int seconds;

   disable_interrupts(GLOBAL);

   i2c_start();

   // if i2c_write does not return 0 -> it means we did not get an ACK back
   // from the slave. It means the slave is not present or is not working.

   if (i2c_write(TARGET_ADDRESS | 0) !=0) {
      resetI2C();  // reset the bus (this is optional)
      enable_interrupts(GLOBAL);
      return(NOT_SUCCESS);
      
   }

   i2c_write(0x00);      // REG 0 
   i2c_start();
   i2c_write(TARGET_ADDRESS | 1);      // RD from RTC 
   seconds = i2c_read(0); // Read current "seconds" in DS1307 
   i2c_stop(); 
   seconds &= 0x7F; 

   delay_us(5); 

   i2c_start(); 
   i2c_write(TARGET_ADDRESS | 0);      // WR to RTC 
   i2c_write(0x00);      // REG 0 
   i2c_write(seconds);     // Start oscillator with current "seconds value 
   i2c_start(); 
   i2c_write(TARGET_ADDRESS | 0);      // WR to RTC 
   i2c_write(0x07);      // Control Register 
   i2c_write(0x80);     // Disable squarewave output pin 
   i2c_stop();  

   enable_interrupts(GLOBAL);

   return(SUCCESS);
 
 
 
 
}

void ds1307_set_date_time(BYTE day, BYTE mth, BYTE year, BYTE dow, BYTE hr, BYTE min, BYTE sec) 
{ 
  sec &= 0x7F; 
  hr &= 0x3F; 

  i2c_start(); 
  i2c_write(0xD0);            // I2C write address 
  i2c_write(0x00);            // Start at REG 0 - Seconds 
  i2c_write(bin2bcd(sec));      // REG 0 
  i2c_write(bin2bcd(min));      // REG 1 
  i2c_write(bin2bcd(hr));      // REG 2 
  i2c_write(bin2bcd(dow));      // REG 3 
  i2c_write(bin2bcd(day));      // REG 4 
  i2c_write(bin2bcd(mth));      // REG 5 
  i2c_write(bin2bcd(year));      // REG 6 
  i2c_write(0x80);            // REG 7 - Disable squarewave output pin 
  i2c_stop(); 
} 

void ds1307_get_date(BYTE *day, BYTE *mth, BYTE *year, BYTE *dow) 
{ 
  i2c_start(); 
  i2c_write(0xD0); 
  i2c_write(0x03);            // Start at REG 3 - Day of week 
  i2c_start(); 
  i2c_write(0xD1); 
  *dow  = bcd2bin(i2c_read() & 0x7f);   // REG 3 
  *day  = bcd2bin(i2c_read() & 0x3f);   // REG 4 
  *mth  = bcd2bin(i2c_read() & 0x1f);   // REG 5 
  *year = bcd2bin(i2c_read(0));            // REG 6 
  i2c_stop(); 
} 

void ds1307_get_time(BYTE *hr, BYTE *min, BYTE *sec) 
{ 
  i2c_start(); 
  i2c_write(0xD0); 
  i2c_write(0x00);            // Start at REG 0 - Seconds 
  i2c_start(); 
  i2c_write(0xD1); 
  *sec = bcd2bin(i2c_read() & 0x7f); 
  *min = bcd2bin(i2c_read() & 0x7f); 
  *hr  = bcd2bin(i2c_read(0) & 0x3f); 
  i2c_stop(); 

} 


int readTwoBytesFromSlave() {
   BYTE hr, min,sec;
   BYTE yr, mo, day, dow;

   disable_interrupts(GLOBAL);

   i2c_start();

   // if i2c_write does not return 0 -> it means we did not get an ACK back
   // from the slave. It means the slave is not present or is not working.

   if (i2c_write(TARGET_ADDRESS | 0) !=0) {
      resetI2C();  // reset the bus (this is optional)
      enable_interrupts(GLOBAL);
      return(NOT_SUCCESS);
      
   }

  
   // send a command to the slave telling it that 
   // we want to read two bytes from it.
   i2c_write(CMD_READ_TWO_BYTES_FROM_SLAVE);
   
   // We re-start the I2C to switch it to read from the slave
   // *Note: there must not be an i2c_stop() before this command
   //        even if it is inside an "if". The compiler will think
   //        we have stopped the i2c and i2c_start will not 
   //        generat a re-start condition on the bus. This will
   //        cause i2c communication errors 
   i2c_start();
   
   // tell the slave we want to read from it by setting bit 0 of the slave
   // address to 1.
   i2c_write(TARGET_ADDRESS | 1);
    
   char buf[64];
   char dateTime[32];
   char date[2];
   char month[2];
   char hour[2];
   char minute[2];
   
   int index = 0;
   i2c_read();
   char ch = 0;
   int i;
   for ( i = 0; i < 20; i += 1)
   {
     buf[i] = i2c_read();
   }
   i2c_read(0);
   
   i2c_stop();
   
   //time
   initDS1307();
   delay_ms(1000);      
    ds1307_get_time(&hr, &min, &sec);
    ds1307_get_date(&day, &mo, &yr, &dow);
     
   sprintf(dateTime,"&D=%02u%02u%02u_%02u%02u",yr,mo,day,hr,min);
   dateTime[15]=0;
   
   buf[20] = '&';
   buf[21] = 'M';
   buf[22] = '=';
   buf[23] = '1';
  
    for ( i = 0; i < 15; i += 1)
   {
     buf[i+24] = dateTime[i];
   }
  
   buf[39] = 0;
   
   
   //printf(usb_cdc_putc, ">>> %s\r\n", buf);
   //printf(usb_cdc_putc, ">>> %s\r\n", dateTime);
   //printf(usb_cdc_putc," Time: %u:%u:%u  Date: <%u> %u/%u/%u  \r\n",hr,min,sec,dow,mo,day,yr);
      
   //char msg [] = "M=1&D=120823_2153&V=12.5&T=35.2&H=50.1&S=3.5&P=3.48&R=56.1&N=28";
   delay_ms(10000);
   send_LETTER_to_XBEE(buf);
   delay_ms(10000);
   
   enable_interrupts(GLOBAL);
   return(SUCCESS);   
}


void main()
{
   int count = 0;
   BYTE hr, min,sec;
   BYTE yr, mo, day, dow;
   int16 clock_tran;

   // FIXME : Not sure if this enough or not??!!
   
   //enable_interrupts(INT_SSP);
   //enable_interrupts(GLOBAL);
      
   //------- RK program, send to GSM
   //char msg [] = "M=1&D=120823_2153&V=12.5&T=35.2&H=50.1&S=3.5&P=3.48&R=56.1&N=28";
   //delay_ms(10000);
   //send_LETTER_to_XBEE(msg);
   //delay_ms(10000);
   //-----------------------------  
   
 
   //usb_init();  // initialize the USB port
   //while(!usb_cdc_connected()) {}    // wait for the computer to open the port
   ;
   
           
    while (1)
   {
      readTwoBytesFromSlave();
      delay_ms(1000);
    }
}


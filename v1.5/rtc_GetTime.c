#include <18F4550.h>
//#fuses HSPLL,NOWDT, NOPROTECT,BROWNOUT, BORV45, NOLVP, PUT, NODEBUG, USBDIV, PLL5, CPUDIV1, VREGEN, MCLR, NOXINST, ICPRT,



#FUSES NOWDT                    //No Watch Dog Timer
#FUSES WDT128                   //Watch Dog Timer uses 1:128 Postscale
#FUSES HSPLL                    //High Speed Crystal/Resonator with PLL enabled
#FUSES NOPROTECT                //Code not protected from reading
#FUSES NOBROWNOUT               //No brownout reset
#FUSES BORV45                   //Brownout reset at 4.5V
#FUSES NOPUT                    //No Power Up Timer
#FUSES NOCPD                    //No EE protection
#FUSES STVREN                   //Stack full/underflow will cause reset
#FUSES NODEBUG                  //No Debug mode for ICD
#FUSES NOLVP                    //No low voltage prgming, B3(PIC16) or B5(PIC18) used for I/O
#FUSES NOWRT                    //Program memory not write protected
#FUSES NOWRTD                   //Data EEPROM not write protected
#FUSES IESO                     //Internal External Switch Over mode enabled
#FUSES FCMEN                    //Fail-safe clock monitor enabled
#FUSES PBADEN                   //PORTB pins are configured as analog input channels on RESET
#FUSES NOWRTC                   //configuration not registers write protected
#FUSES NOWRTB                   //Boot block not write protected
#FUSES NOEBTR                   //Memory not protected from table reads
#FUSES NOEBTRB                  //Boot block not protected from table reads
#FUSES NOCPB                    //No Boot Block code protection
#FUSES MCLR                     //Master Clear pin enabled
#FUSES LPT1OSC                  //Timer1 configured for low-power operation
#FUSES NOXINST                  //Extended set extension and Indexed Addressing mode disabled (Legacy mode)
#FUSES PLL5                     //Divide By 5(20MHz oscillator input)
#FUSES CPUDIV1                  //No System Clock Postscaler
#FUSES USBDIV                   //USB clock source comes from PLL divide by 2
#FUSES VREGEN                   //USB voltage regulator enabled
//#FUSES ICPRT                    //ICPRT enabled



#use delay(clock=48000000)


#include <usb_cdc.h>
//#device adc=10
#include <STDLIB.H>
//void rb_isr();

//#use rs232(baud=9600, UART1)

// Useful options (see PIC-C help for a complete list)
//
// SCL = pin, SDA = pin    - Choose the pins manually when using 
//                           software-based I2C.
// STREAM = id             - Give this I2C bus a name. Usful when
//                           using two I2C buses.
// FAST=nnnnnn             - Set I2C baudrate to nnnnnn Hz
//                           Defalt I2C rate is 100 KHz
//                           Fast I2C devices can run at 400 KHz or higher
//                           * only use 100KHz for software-based I2C.
// FORCE_HW                - Tell the compiler to use the PIC's i2c hardware
//                           for the i2c bus. Must use PIN_C3, PIN_C4 on the 16F887.
// FORCE_SW                - Tell the compiler to user software-based I2C.

#use i2c(MASTER, I2C1, FORCE_HW)

// Registers used to check and clear I2C errors
#bit SSPEN = 0x14.5
#bit SSPOV = 0x14.6
#bit WCOL  = 0x14.7

#define TARGET_ADDRESS  0xD0    // DS1307 I2C Address (Not changable)

#define SUCCESS      1
#define NOT_SUCCESS  0

#define CMD_SEND_BYTE_TO_SLAVE         1
#define CMD_READ_TWO_BYTES_FROM_SLAVE  2


void resetI2C() {

   // clear the error flag registers and re-enable the i2c bus
   SSPEN = 0;   // disable i2c
   SSPOV = 0;   // clear the receive overflow flag
   WCOL = 0;    // clear the write collision flag

   SSPEN = 1;   // re-enable i2c   

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



/////////////////////////////////////////////////////////////////////
//  Send one byte to the slave
/////////////////////////////////////////////////////////////////////

int setRegisterPointer(int outByte) {

   disable_interrupts(GLOBAL);

   i2c_start();

   // if i2c_write does not return 0 -> it means we did not get an ACK back
   // from the slave. It means the slave is not present or is not working.

   if ( i2c_write(TARGET_ADDRESS | 0) != 0) {
      resetI2C();  // reset the bus (this is optional)
      enable_interrupts(GLOBAL);

      return(NOT_SUCCESS);
   }

   // set the register pointer 
   i2c_write(outByte);

   i2c_stop();
   enable_interrupts(GLOBAL);

   return(SUCCESS);
   
}


// Clear CH bit
//
// The CH bit is bit 7 at register address 0.
// The bit is set by default when the DS1307 looses power
// We need to clear this bit to enable the chip (making the clock tick).
// We only need to do this once.

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

/////////////////////////////////////////////////////////////////////
//  Read a register value from the DS1307
//
//  Note: It is important that we use a second i2c_start() when
//        we want to switch the data flow from master->slave to 
//        master<-slave. This i2c_start() will actually generate
//        a "re-start" condition on the bus. DO NOT use i2c_stop()
//        before this command. The program will not work.
/////////////////////////////////////////////////////////////////////

int readRTC(int registerAddress, int *registerValue) {

   disable_interrupts(GLOBAL);

   i2c_start();

   // if i2c_write does not return 0 -> it means we did not get an ACK back
   // from the slave. It means the slave is not present or is not working.


   
   if (i2c_write(TARGET_ADDRESS | 0) !=0) {
      resetI2C();  // reset the bus (this is optional)
      enable_interrupts(GLOBAL);
      return(NOT_SUCCESS);
      
   }

  
   // set the register pointer on the DS1307
   i2c_write(registerAddress);
   
   i2c_start();
   
   // tell the slave we want to read from it by setting bit 0 of the slave
   // address to 1.
   i2c_write(TARGET_ADDRESS | 1);
   
   *registerValue = i2c_read(0); // read from the DS1307. Parameter 0 tells i2c_read() 
                           // to do a "not acknowledge" or NACK read which tells the 
                           // DS1307 to end a read transaction. 
                           // Note. Without the NACK, you can continually read
                           // from the DS1307, getting the sequence of register
                           // values.
   i2c_stop();

   enable_interrupts(GLOBAL);

   return(SUCCESS);
   
}


void main() {
   //enable_interrupts(INT_RB);
   //enable_interrupts(GLOBAL);
   
   int setFlag=FALSE;

   BYTE hr, min,sec;
   BYTE yr, mo, day, dow;
   int16 clock_tran;
   
   
  //--  These setup functions are commencted out
  //--  Needed to be rechecked with A. Roger
  //--  Paskorn C. 2/19/11
 /*
   setup_adc_ports(AN0);
   setup_adc(ADC_CLOCK_INTERNAL);
   setup_psp(PSP_DISABLED);
   setup_spi(SPI_SS_DISABLED);
   setup_wdt(WDT_OFF);
   setup_timer_0(RTCC_INTERNAL);
   setup_timer_1(T1_DISABLED);
   setup_timer_2(T2_DISABLED,0,1);
   setup_comparator(NC_NC_NC_NC);
   setup_vref(FALSE);

   set_adc_channel(0);
   int switch1 = read_adc();
   int check1=0;*/

   usb_init(); // initialize the USB port
   while(!usb_cdc_connected()) {} // wait for the computer to open the port

   
   
      
   while (TRUE) {
     
      
      
      if(!setFLAG)
      {
      setFLAG=TRUE;
      printf(usb_cdc_putc, "Real Time Clock Time Checker \r\n");
      
      if (initDS1307())
         printf(usb_cdc_putc,"initDS1307 Success! \r\n");
      else 
         printf(usb_cdc_putc,"initDS1307 Failed");      
         
       printf(usb_cdc_putc,"RTC Current Time(hr::sec)  Date(<dow> m/d/y)> \r\n");
       
       
      }
      
      delay_ms(1000);      
           
        //------------------- Show RTC Time --------------------------------
      ds1307_get_time(&hr, &min, &sec);
      ds1307_get_date(&day, &mo, &yr, &dow);
      
      char s[3];
     // dow2weekday(dow,&s);
      printf(usb_cdc_putc," Time: %u:%u:%u  Date: <%u> %u/%u/%u  \r\n",hr,min,sec,dow,mo,day,yr);
      
    
      clock_tran = ((int16)hr*100)+(int16)min;
      
      i2c_start();
      i2c_write(0xB0); // ??? Address ???????????????????????????????
      i2c_write(2); // ???????????????????
      i2c_write(clock_tran>>8); // byte ???????? 16 bit
      i2c_write(clock_tran); // byte ????
      i2c_stop();
     
    
   } 
   
   
   

}



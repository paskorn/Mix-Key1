#include <16F886.h>
#include <Slave.h>

#use i2c(SLAVE, SDA=PIN_C4, SCL=PIN_C3, address=0xB6, FORCE_HW)
#use rs232(baud = 9600, xmit = PIN_C6, rcv = PIN_C7)


#include <string.h>

// Registers used to check and clear I2C errors
#bit SSPEN = 0x14.5
#bit SSPOV = 0x14.6
#bit WCOL  = 0x14.7

////////////////////////////////////////////////////////////////////////////////  
///
///   G L O B A L    V A R I A B L E S .
///
////////////////////////////////////////////////////////// --- Global Variables.

char  BufferString[MAXSTR]={0};
int   BufferStringIndex = 0;
int   DataReady = 0;

int slaveState = READY_FOR_ADDRESS;
int watchdog=0;

////////////////////////////////////////////////////////////////////////////////  
///
///   F U N C T I O N    D E C L A R A T I O N .

////////////////////////////////////////////////////////////////////////////////  
///
///   M A I N    F U N C T I O N .
///
////////////////////////////////////////////////////////////////////// --- Main.

void main()
{  
   setup_adc_ports(NO_ANALOGS|VSS_VDD);
   setup_adc(ADC_CLOCK_DIV_2);
   //setup_spi(SPI_SS_DISABLED);
   setup_timer_0(RTCC_INTERNAL|RTCC_DIV_1);
   setup_timer_1(T1_DISABLED);
   setup_timer_2(T2_DISABLED,0,1);
   setup_ccp1(CCP_OFF);
   setup_comparator(NC_NC_NC_NC);// This device COMP currently not supported by the PICWizard

   enable_interrupts(INT_SSP);
   enable_interrupts(GLOBAL);
   //TODO: User Code
   
   while(1)
   {
      if(DataReady){
         DataReady = 0;
         
         if (strlen(BufferString) > 0)
         {
           puts("AT+IPR?\r"); // Baudrate
           delay_ms(200);
           
           
           puts("AT+COPS?\r"); // Provider
           delay_ms(200);  
           
           
           puts("AT+CMGF=1\r"); // SMS Message = Text Mode
           delay_ms(200);  
           
           
           puts("AT+CSCLK=0\r"); // Diable Sleep Mode
           delay_ms(200);  
          
           
           puts("AT+CSQ\r"); // Test Signal
           delay_ms(200);  
          
           
           puts("AT+CMGS=\"+66804986837\"\r"); // Phone Number
           delay_ms(200);  
           
           printf("%s\r",BufferString); // Message
           delay_ms(200);  


           putc(0x1a); // Send 
           BufferString[0] = 0;
         }
      }
      delay_us(50);
   }
}

////////////////////////////////////////////////////////////////////////////////  
///
///   I N T E R R U P T    S E R V I C E    R O U T I N E S
///
//////////////////////////////////////////////// --- Interrupt Service Routines.


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

   //////////////////////////////////////////////////////////////////
   //  Address match with bit 0 clear (master write to slave)
   //////////////////////////////////////////////////////////////////
   if (state == 0) {
      i2c_read();  // flush the device address in the rx buffer
      

      // Fix any errors in the I2C communication
      // if wrong state -> there must have been an error in the i2c comm
      if (slaveState != READY_FOR_ADDRESS) {
         // clear the error flag registers and re-enable the i2c bus
         SSPEN = 0;   // disable i2c
         SSPOV = 0;   // clear the receive overflow flag
         WCOL = 0;    // clear the write collision flag

         SSPEN = 1;   // re-enable i2c

         slaveState = READY_FOR_ADDRESS; // reset the state
         return;

      }
   
      // if no error -> move forward to the next state
      slaveState=READY_FOR_CMD;     //device moves into command mode

   }

   //////////////////////////////////////////////////////////////////
   //  a byte has been recieved from the Master
   //////////////////////////////////////////////////////////////////

   else if (state < 0x80) {
      inByte=i2c_read();
      watchdog=0;

      switch (slaveState) {        
      
        case READY_FOR_CMD:
      
            switch (inByte) {
               case PING:
                  slaveState=READY_FOR_ADDRESS;       // Just a ping. Do nothing
                  break;
               case DISPLAY_TEXT:
                  slaveState=READY_FOR_LETTER;        //first step of text display routine; can also display numbers
                  break;
               default:
                  slaveState = READY_FOR_ADDRESS;
                  break;
            }
   
            break;
      
         case READY_FOR_LETTER:
            if(inByte != '\0')
            {
               BufferString[BufferStringIndex] = inByte;
               BufferStringIndex++;
               BufferStringIndex %= MAXSTR;
                              
               slaveState=READY_FOR_LETTER;
            }
            else
            {
               DataReady = 1;
               BufferStringIndex = 0;
               slaveState=READY_FOR_ADDRESS;
            }
            break;                     
      }
         
   } 
   else 
   {
      slaveState = READY_FOR_ADDRESS;
   }
}

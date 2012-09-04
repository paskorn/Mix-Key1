#include <16F886.h>


#include <Slave.h>
//#device adc=10

#use delay(clock=20000000)
#use i2c(SLAVE, SDA=PIN_C4, SCL=PIN_C3, address=0xB0, FORCE_HW)
//9#use rs232(baud = 9600, xmit = PIN_C6, rcv = PIN_C7)

#FUSES NOWDT //No Watch Dog Timer
#FUSES HS //High speed Osc (> 4mhz for PCM/PCH) (>10mhz for PCD)
#FUSES NOPUT //No Power Up Timer
#FUSES MCLR //Master Clear pin enabled
#FUSES NOPROTECT //Code not protected from reading
#FUSES NOCPD //No EE protection
#FUSES NOBROWNOUT //No brownout reset
#FUSES IESO //Internal External Switch Over mode enabled
#FUSES FCMEN //Fail-safe clock monitor enabled
#FUSES NOLVP //No low voltage prgming, B3(PIC16) or B5(PIC18) used for I/O
#FUSES NODEBUG //No Debug mode for ICD
#FUSES BORV40 //Brownout reset at 4.0V
#FUSES NOWRT //Program memory not write protected

#include <math.h>
#include <oasysLCD.c>
#include <SHT15.c>
#include <myMCP3208.c>
#include <ultrasonic.c>

// global variables
int slaveState1=READY_FOR_ADDRESS;   // state machine variable
int valFromMaster=0;  // buffer for the byte received from the master
int sentCount=0;  // tracks the number times slave has sent data to the master


int watchdog=0;

float vTemp=0;
float vHumid=0;
float vSonic=0;

void send_letter_to_MASTER(char* text);

void main(void){
   int count = 0;
   InitialChip();
   delay_ms(200);
   SHTConReset(); 
   set_tris_c(0b00011001);
   init();
   
   //Pressure_init();
   output_low(PIN_A0);
   
  //1 setup_adc_ports(NO_ANALOGS|VSS_VDD);
  //2 setup_adc(ADC_CLOCK_DIV_2);
   //setup_spi(SPI_SS_DISABLED);
  //3 setup_timer_0(RTCC_INTERNAL|RTCC_DIV_1);
  //4 setup_timer_1(T1_DISABLED);
  //5 setup_timer_2(T2_DISABLED,0,1);
  //6 setup_ccp1(CCP_OFF);
  //7setup_comparator(NC_NC_NC_NC);// This device COMP currently not supported by the PICWizard

   enable_interrupts(INT_SSP);
   enable_interrupts(GLOBAL);
   
    
   if (gblTimeToUpdateScreen) 
   {
      gblTimeToUpdateScreen = 0;
      updateScreen();
   }
   
   while(true)
   {               
     float temp, humid;
     //int16 sonic;
     float sonic;
     
     int error = oa_Temp_n_Humid(temp, humid);
     if (error) continue;
     //temp =3265;
     //humid =6532;
     show_temp(temp);
     show_humid(humid);
     
     //PC commented to change to Float
     //sonic = oa_Sonic();
     //show_sonic(sonic);
     
     
     sonic = foa_Sonic();
     //sonic = 6000/16-sonic;
     show_sonic(sonic);
     
     
     vTemp = temp;
     vHumid = humid;
     vSonic = sonic;
   }
}


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
      if (slaveState1 != READY_FOR_ADDRESS) {
         // clear the error flag registers and re-enable the i2c bus
         SSPEN = 0;   // disable i2c
         SSPOV = 0;   // clear the receive overflow flag
         WCOL = 0;    // clear the write collision flag

         SSPEN = 1;   // re-enable i2c

         slaveState1 = READY_FOR_ADDRESS; // reset the state
   

         return;

      }
   
      // if no error -> move forward to the next state
      slaveState1=READY_FOR_CMD;     //device moves into command mode

   }

   //////////////////////////////////////////////////////////////////
   //  a byte has been recieved from the Master
   //////////////////////////////////////////////////////////////////

   else if (state < 0x80) {

         inByte=i2c_read();

         switch (slaveState1) {

            case READY_FOR_CMD:

               switch (inByte) {
               
                  case CMD_RECEIVE_A_BYTE_FROM_MASTER:
                     slaveState1=READY_FOR_NUMBER;  
                     break;
                  case CMD_SEND_TWO_BYTES_TO_MASTER:
                     slaveState1=READY_TO_SEND; 
                     break;

                  // reset the state if received an
                  // unknown command
                  default:
                     slaveState1 = READY_FOR_ADDRESS;
                     break;
               }

               break;

            case READY_FOR_NUMBER:
               valFromMaster=inByte;
               slaveState1=READY_FOR_ADDRESS;
               break;

            // reset the state if unknown state is detected
            default:
               slaveState1=READY_FOR_ADDRESS;
               break;

         }

   }

   //////////////////////////////////////////////////////////////////
   //  Address match with bit 0 set (slave write to master)
   //////////////////////////////////////////////////////////////////

   else if (state == 0x80) {


      i2c_read();  // flush the address byte
                     
      if (slaveState1 == READY_TO_SEND) {

            i2c_write(0x61);
            slaveState1=READY_TO_SEND_BYTE_2;
      }

   }

   //////////////////////////////////////////////////////////////////
   //  Master is ready to receive the next byte from slave
   //////////////////////////////////////////////////////////////////

   else if (state > 0x80) {

      if (slaveState1 == READY_TO_SEND_BYTE_2) {
            //i2c_write(222);
           
            char buf [32];
            memset(buf, 0, 32);
            
            vTemp = vTemp/100;
            vHumid = vHumid/100;
            
            //PC changed /100 from 1000 1:21
            vSonic = vSonic/16;
            vSonic = vSonic/100;
            vSonic = 6-vSonic; 
            
            //vTemp = 3.2;
            sprintf(buf, "T=%.1f&H=%.1f&S=%.2f", vTemp, vHumid, vSonic);
            int len = 20;//strlen(buf);
            int i;
            for(i = 0; i < len; i += 1)
            {
               i2c_write(buf[i]);
            }
            i2c_write('\0');
            i2c_write('\0');
            slaveState1=READY_FOR_ADDRESS;
            sentCount++;
      }
   }


}



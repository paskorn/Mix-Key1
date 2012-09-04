/*
#include <16F886.h>
#include <math.h>
#include <oasysLCD.c>
//#device adc=10

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

#use delay(clock=20000000)
*/


#define SHT1xDATA  PIN_A2
#define SHT1xSCK   PIN_A3
#define noACK 0 
#define ACK   1 

// SHT1x address=000 is currently supported 
// SHT1x command code 
                            //adr  command  r/w 
#define STATUS_REG_W 0x06   //000   0011    0 
#define STATUS_REG_R 0x07   //000   0011    1 
#define MEASURE_TEMP 0x03   //000   0001    1 
#define MEASURE_HUMI 0x05   //000   0010    1 
#define RESET        0x1E   //000   1111    0 

// constant use for SHT1x Humidity Measurement 
#define C1  -4.0 
#define C2  0.0405 
#define C3  -0.0000028 

// constant use for SHT1x Temperature Measurement 
#define D1  -40.0 
#define D2  0.01 

// constant use for SHT1x True Humidity Measurement 
#define T1  0.01 
#define T2  0.00008 

//PIC16F6X8
//#define  CMCON   0x1F
//#define  PORTA    0x05
//#define  PORTB    0x06


void InitialChip(void)
{
   setup_comparator(NC_NC_NC_NC);   //Input Digital
   set_tris_b(0B00000000);
   set_tris_c(0B10000000);
}

//SHT1x Transmission Start condition
void SHTStart()
{
   output_high(SHT1xDATA);
   output_low(SHT1xSCK);
   output_high(SHT1xSCK);
   output_low(SHT1xDATA);
   output_low(SHT1xSCK);
   output_high(SHT1xSCK);
   output_high(SHT1xDATA);
   output_low(SHT1xSCK);
}

// SHT1x Connection Reset:
void SHTConReset() 
{ 
   int i; 
   
   output_high(SHT1xDATA); 
   
   for (i=0; i<9; i++) 
   { 
      output_high(SHT1xSCK); 
      delay_us(2); 
      output_low(SHT1xSCK); 
      delay_us(2); 
   } 
   SHTStart(); 
} 

// SHT1x Address & Command Mode with address=000 
int SHTWrite(int Data) 
{ 
   int i; 
   
   for (i=0x80;i>0;i/=2)        //shift bit for masking data
   { 
      if(i&Data)
      output_high(SHT1xDATA); 
      else  
      output_low(SHT1xDATA); 
      
      delay_us(2);               //Snend Clock each bit 
      output_high(SHT1xSCK); 
      delay_us(2); 
      output_low(SHT1xSCK); 
   } 
   
   output_float(SHT1xDATA);     //Change DATA Line to Input 
   delay_us(2); 
   
   output_high(SHT1xSCK);       //Clock for Acknowledge 
   delay_us(2); 
   
   i= input(SHT1xDATA);         //Get Acknowledge
   
   output_low(SHT1xSCK); 
   delay_ms(250);
   return (i);
} 

//Read data from SHT1x
long SHTRead(void) 
{ 
   int i; 
   long lTmp,lVal1,lVal2,lValue; 
   
   lVal1=0;
   lVal2=0; 
   
   //get MSB from SHT1x 
   for (i=0; i<8; i++) 
   { 
      lVal1<<=1; 
      output_high(SHT1xSCK);          //Send Clock Hight
      lTmp = input(SHT1xDATA);        //Read Data Bit 
      output_low(SHT1xSCK);           //Send Clock Low
      
      if(lTmp) 
         lVal1|=1;                      //store in lVal1
      } 
   
   //Acknowledge routine for Next byte
   output_low(SHT1xDATA); 
   output_high(SHT1xSCK); 
   
   output_float(SHT1xDATA);         //Change to Input
   output_low(SHT1xSCK); 
   
   //get LSB from SHT1x 
   for (i=0; i<8; i++) 
   { 
      lVal2<<=1; 
      output_high(SHT1xSCK);          //Send Clock Hight 
      lTmp = input(SHT1xDATA);        //Read Data Bit  
      //delay_us(2); 
      output_low(SHT1xSCK);           //Send Clock Low
      //delay_us(2); 
      
      if(lTmp) 
         lVal2|=1;                     //store in lVal2
   }    
   
   lValue = make16(lVal1,lVal2);    //Makes a 16 bit number out of two 8 bit numbers.
   return(lValue); 
} 
// SHT1x Soft Reset 
// resets the interface, clears the status register to default values 
// wait minimum 11ms before next command 
void SHTSoftReset() 
{ 
  SHTConReset(); 
  SHTWrite(RESET); 
} 

// calculate dewpoint 
float sht1x_calc_dewpoint(float fRh,float fTemp) 
{ 
   float fDewpoint; 
   float fLogEW; 

   fLogEW = ((7.5*fTemp)/(237.3+fTemp))+log10(fRh)-1.33923; 
   fDewpoint = ((0.66077-log10(fLogEW))*(237.3))/(log10(fLogEW)-8.16077); 
   return(fDewpoint); 
} 

float oa_Temp(void)
{
   float fTemp_true; 
   long lValue_temp; 
   int R;

   SHTStart();                            //@1 start transmission 
   R=SHTWrite(MEASURE_TEMP);           //@2 measure temperature 
   if(R==1)
   {
      //printf("Sensor Error\n\r");
      delay_ms(1000);
   }
   
   lValue_temp = SHTRead(); 
   
   // temperature calculation 
   fTemp_true = (D1+(D2*lValue_temp)); 
   fTemp_true = fTemp_true * 100; //float ot int
   // delay 11ms before next command 
   delay_ms(12);
   
   return(fTemp_true);
}

int oa_Temp_n_Humid(float& temp, float& humid)
{
   float fRh_lin; 
   float fTemp_true; 
   float fRh_true;

   long lValue_rh; 
   long lValue_temp; 
   int R;
   
   SHTStart();                         //@1 start transmission 
   R=SHTWrite(MEASURE_TEMP);           //@2 measure temperature 
   if(R==1){
      delay_ms(1000);
      return 1;
   }
   
   lValue_temp = SHTRead(); 
   
   // temperature calculation 
   fTemp_true = (D1+(D2*lValue_temp)); 
   
   // delay 11ms before next command 
   delay_ms(12); 
   
   // start transmission 
   SHTStart(); 
   
   // measure relative humidity 
   SHTWrite(MEASURE_HUMI);
   lValue_rh = SHTRead(); 

   // relative humidity calculation 
   fRh_lin = (C1+(C2*lValue_rh)+(C3*lValue_rh*lValue_rh)); 
   fRh_true = (((fTemp_true-25)*(T1+(T2*lValue_rh)))+fRh_lin);
   
   temp = (fTemp_true * 100);
   humid = (fRh_true * 100);
   return 0;
}

/*
// Main Program 
void main(void) { 
   float fRh_lin; 
   float fRh_true; 
   float fTemp_true; 
   float fDew_point; 

   long lValue_rh; 
   long lValue_temp; 
   int R;

   InitialChip();
   delay_ms(200);

   SHTConReset(); 

   //usb_init();

   set_tris_c(0b00011001);
   init();
   
   
   while (TRUE) 
   { 
//      show_temp(oa_Temp());
      float temp, humid;
      int error = oa_Temp_n_Humid(temp, humid);
      if (error) continue;
      show_temp(temp);
      //delay_ms(1000); 

      SHTStart();                         //@1 start transmission 
      R=SHTWrite(MEASURE_TEMP);           //@2 measure temperature 
         if(R==1){
            //printf("Sensor Error\n\r");
            //printf("Sensor Error\n");
            delay_ms(1000);
            continue;
         }

      lValue_temp = SHTRead(); 

      // temperature calculation 
      fTemp_true = (D1+(D2*lValue_temp)); 

      // delay 11ms before next command 
      delay_ms(12); 

      // start transmission 
      SHTStart(); 

      // measure relative humidity 
      SHTWrite(MEASURE_HUMI); 
      lValue_rh = SHTRead(); 

      // relative humidity calculation 
      fRh_lin = (C1+(C2*lValue_rh)+(C3*lValue_rh*lValue_rh)); 
      fRh_true = (((fTemp_true-25)*(T1+(T2*lValue_rh)))+fRh_lin); 

      // dewpoint calculation 
      fDew_point = sht1x_calc_dewpoint(fRh_true,fTemp_true); 
   
      //printf("T=%3.2f C\r\n",fTemp_true);
      //printf("H=%3.2f%%\r\n",fRh_true);
      //printf("Dew Point: %3.6fC \r\n\r\n",fDew_point);
      
      show_temp(fTemp_true*100);
   }
} 
*/

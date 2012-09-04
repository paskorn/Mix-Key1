//!#include <16F886.h>
//!#include <math.h>
//!
//!//#device adc=10
//!
//!#FUSES NOWDT //No Watch Dog Timer
//!#FUSES HS //High speed Osc (> 4mhz for PCM/PCH) (>10mhz for PCD)
//!#FUSES NOPUT //No Power Up Timer
//!#FUSES MCLR //Master Clear pin enabled
//!#FUSES NOPROTECT //Code not protected from reading
//!#FUSES NOCPD //No EE protection
//!#FUSES NOBROWNOUT //No brownout reset
//!#FUSES IESO //Internal External Switch Over mode enabled
//!#FUSES FCMEN //Fail-safe clock monitor enabled
//!#FUSES NOLVP //No low voltage prgming, B3(PIC16) or B5(PIC18) used for I/O
//!#FUSES NODEBUG //No Debug mode for ICD
//!#FUSES BORV40 //Brownout reset at 4.0V
//!#FUSES NOWRT //Program memory not write protected
//!
//!#use delay(clock=20000000)
//#use RS232 (baud=9600, xmit=PIN_C6, rcv=PIN_C7)

//#include <oasysLCD.c>
//#include <myMCP3208.c>
//#include <SHT15.c>
//#include <Pressure.c>
//#include <usb_cdc.h>

int16 oa_Sonic(void){
  int16 value_sonic = 0;
  int16 total_sonic = 0;
  int i;
  for( i=1;i<=10;i++){
      value_sonic = read_analog(0);
      total_sonic = total_sonic + value_sonic;
  }
  total_sonic = total_sonic / 10; //avg loop 100
  total_sonic = total_sonic / 4;  //12 Bit to 10 Bit
  return(total_sonic);
}


float foa_Sonic(void){
  float value_sonic = 0;
  float total_sonic = 0;
  int i;
  for( i=1;i<=10;i++){
      value_sonic = read_analog(0);
      total_sonic = total_sonic + value_sonic;
  }
  total_sonic = total_sonic / 10; //avg loop 100
  total_sonic = total_sonic / 4;  //12 Bit to 10 Bit
  return(total_sonic);
}



//!void main(void){
//!   
//!   set_tris_c(0b00011001);
//!   init();
//!   
//!   int16 value_sonic = 0;
//!
//!   while(true){
//!     value_sonic = read_analog(1);
//!     value_sonic = value_sonic / 4;
//!     show_water(value_sonic);
//!   }
//!}

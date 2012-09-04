#include <18F4550.h>


#device adc=10
#use delay(clock=48000000)
#use i2c(MASTER, I2C1, FORCE_HW)

#include <usb_cdc.h>
#include <stdlib.h> 
#include <STRING.H>
#include <math.h>
//#include <XBEE_I2C.h>

#FUSES NOWDT                    //No Watch Dog Timer
//#FUSES WDT128                   //Watch Dog Timer uses 1:128 Postscale
#FUSES HSPLL                    //High Speed Crystal/Resonator with PLL enabled
#FUSES NOPROTECT                //Code not protected from reading
#FUSES NOBROWNOUT               //No brownout reset
//#FUSES BORV45                   //Brownout reset at 4.5V
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
// PC Commented
//#FUSES ICPRT                    //ICPRT enabled



////////////////////////////////////////////////////////////////////////////////  
///
///   D E F I N E    V A L U E S .
///
///////////////////////////////////////////////////////////// --- Define Values.

// I2C ADDRESS.
#define DISPLAY_ADDRESS 0xB4
#define XBEE_ADDRESS    0xB6

#define DISPLAY_CMD_SEND_VALUE      2
#define DISPLAY_CMD_SEND_LONG_TEXT  5
#define DISPLAY_CMD_CLS             6
#define DISPLAY_CMD_SETPOS          8
#define SEND_CHAR                0x03
#define SEND_FLOAT               0x07

//char between sensor value when send to XBEE.
#define SPACE     ","

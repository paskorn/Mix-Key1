#device adc=8

#FUSES NOWDT                    //No Watch Dog Timer
#FUSES HS                       //High speed Osc (> 4mhz for PCM/PCH) (>10mhz for PCD)
#FUSES NOPUT                    //No Power Up Timer
#FUSES MCLR                     //Master Clear pin enabled
#FUSES NOPROTECT                //Code not protected from reading
#FUSES NOCPD                    //No EE protection
#FUSES NOBROWNOUT               //No brownout reset
#FUSES IESO                     //Internal External Switch Over mode enabled
#FUSES FCMEN                    //Fail-safe clock monitor enabled
#FUSES NOLVP                    //No low voltage prgming, B3(PIC16) or B5(PIC18) used for I/O
#FUSES NODEBUG                  //No Debug mode for ICD
#FUSES BORV40                   //Brownout reset at 4.0V
#FUSES NOWRT                    //Program memory not write protected

#use delay(clock=20000000)



////////////////////////////////////////////////////////////////////////////////  
///
///   D E F I N E    V A L U E S .
///
///////////////////////////////////////////////////////////// --- Define Values.

// commands
#define PING            1
#define DISPLAY_NUMBER  2
#define DISPLAY_TEXT    3
#define UPDATE_SENSORS  4
#define DISPLAY_FLOAT   7

// slave states
#define READY_FOR_ADDRESS    1
#define READY_FOR_CMD        2
//#define READY_TO_ACK         3
#define READY_FOR_LETTER      6


#define DISP_STATE_USER          0
#define DISP_STATE_OFF           1
#define DISP_STATE_SHOW_SENSOR   3

#define TIMEOUT                  10    // 10 is about 1 second
#define BANNER_DISPLAY_DURATION  10    // 10 is about 1 second


// slave states
#define READY_FOR_ADDRESS     1
#define READY_FOR_CMD         2
#define READY_FOR_NUMBER      3
#define READY_TO_SEND         4
#define READY_TO_SEND_BYTE_2  5

// Commands from the master
#define CMD_RECEIVE_A_BYTE_FROM_MASTER  1
#define CMD_SEND_TWO_BYTES_TO_MASTER  2

// Buffer
#DEFINE MAXSTR    64
#DEFINE NUMBER    0
#DEFINE LETTER    1
#DEFINE FLO       2


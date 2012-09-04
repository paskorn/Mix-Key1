//*--
// This code is for PIC16F886 burning
// It is a slave on I2C communication (Go-go board is a master)
// This PIC16F886 must be socketed on the pink board (LCD controller)
// while the LCD module is on the top and the gogo board is under.
//
//  --- 5/2/11 Bug fix - Roger
//      * dirty character algorithm has been debugged. It now works well.
//      * showNumber int16 takes 300usec to complete, causing an overrun
//        error when the i2c master sends commands too quickly. The gogo
//        firmware has been updated to add a delay to make sure this does
//        not happen.
//
//  --- 4/24/11 Code restructuring - Roger
//      * scrren updates takes place only on dirty characters (changed since
//        the last screen update).
//      * Supports sensor update stream from the gogoboard. But this info
//        still cannot be shown on the screen.
//      * Code support for display-short-text (used on 7-segment displays)
//        but still does not work
//      * Code has been restructured 
//
//  --- 4/15/11 Changed constant defs - Roger
//      Changed constants to make the display module compatible with
//      The I2C commands for the 7-segment module. 
//
//  --- 3/24/11 Modified for the new LCD PCB v1.1 - Roger
//      * used defined declaration for pin assignment and i2c address instead of 
//        hard-coded values.
//      * changed i2c address from 0xb2 to 0xb4 as defined in the i2c address allocation document
//      * changed EN,RW,RS pins on the PIC to match those used on the v 1.1 PCB

//  --- 3/10/11 Work Well and stable - PC
//       * handler I2C command HIDECUR, SHOWCUR, GETPOS, SETPOS, CLEAR, DISPLAY 
//       * refresh is used to stop timer interupt after display whole text
//
//  --- 2/26/11 Modification - PC
//       * include to be the right IC (not 16F877A)
//       * the initial screen

//*

#define DEBUG_ON 0      // 1 = debug enabled -> will show error codes on the lcd screen


//#include <16F886.H>


#use fast_io(C)


#define I2C_ADDRESS  0xB4

#define PIN_EN PIN_C7   // Enable signal
#define PIN_RS PIN_C5   // register selection (H=data register, L=instruction register)
#define PIN_RW PIN_C6   // Read/Write selection (H=Read, L=Write)


#define T1_COUNTER 54000

//command
// The first 5 commands are compatible with both the 7-segment and lcd character displays
#define DISPLAY_CMD_PING  1
#define DISPLAY_VALUE 2
#define DISPLAY_SHORT_TEXT   3     // shows a 4 letter text. This is here to provide compatibility with the 7-segment display
#define DISPLAY_UPDATE_SENSORS 4
#define DISPLAY_LONG_TEXT 5

// These commands are specific to the LCD character display
#define CLEAR 6
#define GETPOS 7
#define SETPOS 8
#define HIDECUR 9
#define SHOWCUR 10

#define NOOP 99

//STAT
#define WAIT_ADDRESS 0
#define WAIT_CMD 1
#define WAIT_POSITION 2
#define WAIT_CHARACTOR 3
#define SEND_POSITION 4
#define WAIT_VALUE_HIGH_BYTE 5
#define WAIT_VALUE_LOW_BYTE 6
#define READY_FOR_SENSOR_HI      7
#define READY_FOR_SENSOR_LOW     8
#define WAIT_SHORT_TEXT1      9
#define WAIT_SHORT_TEXT2      10
#define WAIT_SHORT_TEXT3      11
#define WAIT_SHORT_TEXT4      12

// Error codes
#define ERR_UNKNOWN_COMMAND 0    // unknown I2C command
#define ERR_UNKNOWN_STATE 1      // unknown I2C state
#define ERR_WRONG_STATE 2        // wrong I2C idle state (i2c reset will tak place)

// I2C registers -> used to reset i2c
#bit SSPEN = 0x14.5
#bit SSPOV = 0x14.6
#bit WCOL  = 0x14.7

#fuses HS,NOWDT,NOPROTECT, BROWNOUT, PUT, NOMCLR
#use delay (clock=20000000)

#include <stdlib.H>
//#include <MCP3208.c>

//#use rs232(baud=9600, xmit=PIN_C0,rcv=PIN_C1, FORCE_SW)  // debugging purposes


void resetI2C();
void submit();
void type(int code);
void fillBlankSensorsWithDefaultValue(void);
void triggerScreenUpdate();
void clearScreen();
void showCursor();
void hideCursor();
void twoDisplay();
void setPosition(int pos);
int getPosition();
void showError(int errCode, int data);
void init();
void updateScreen();
void main();

//static int setCursor =0; 
int inputCursor=0; //input cursor position 
///static int outputCursor = 1; // current cursur position



static int slaveState = WAIT_ADDRESS; // start state
int input = 0;
//int cmd = NOOP;
int gblDisplayBufferIndex=0;  
int gblDisplayModuleCursorPos=0;
int1 gblTimeToUpdateScreen=0;  // flag to indicate when to update the screen

char curText[0x21]="                                ";
int gblNewLCDPos =0;

int16 gblDisplayValue=0;
char valueBuffer[6]="     ";  // buffer to hold the text version of the received 16 bit display value

/// varialbes used to receive sensor values from the gogo board
int gblSensorBufferHi;
int gblSensorBufferLow;
int16 gblSensorValues[8];

int gblSensorTimeout = 0;
int gblUpdatedSensors = 0;  // keeps log of which sensors values were received
int gblLastSensorReceived; // logs the latest sensor port number received

int32 gblDirtyBits = 0xffffffff;  // flags the characters that have changed

// variables used to tranform int16 into a string.
//!int tenThousands;
//!int thousands;
//!int hundreds;
//!int tens;
//!int ones;

char dis0;  // 1
char dis1;  // 10
char dis2;  // 100
char dis3;  // 1000
char dis4;  // 10000
//char dis5;  // 100000
char disf1; // 0.1
char disf2; // 0.01


// Timer1 triggers about every 4.6 ms. (20MHz DIV_BY_2 and timer = 54000)

#int_timer1     
void timer1_isr(void) {  
   
   gblTimeToUpdateScreen = 1;  // signals main() to update the screen
   set_timer1(T1_COUNTER);

}


void triggerScreenUpdate() {
   // may need to set a flag so that we don't re-trigger while
   // the previous update has not completed

   gblDisplayBufferIndex=0;
   
//   gblNeedToUpdateScreen=1;  // this flag garantees that the 
   enable_interrupts(INT_TIMER1);
}

void resetI2C() {
         // clear the error flag registers and re-enable the i2c bus
         SSPEN = 0;   // disable i2c
         SSPOV = 0;   // clear the receive overflow flag
         WCOL = 0;    // clear the write collision flag

         SSPEN = 1;   // re-enable i2c

}

void submit(){

   output_high(PIN_EN);
   delay_us(50);
   output_low(PIN_EN);
}

void type(int text){ // convert a charactor to ascii

   //show a char on LCD
   output_high(PIN_RS);
   if (text=='\0') { text = ' '; }
   output_b(text);
   submit();   
   output_low(PIN_RS); 
   gblDisplayModuleCursorPos++;  // update var that tracks the display cursor pos
   
   //position updating
   ///outputCursor++;
   ///if(outputCursor==32){  //the last position
                           //this case happens when text is too long(>0x32)
   ////setPosition(0);
   ///outputCursor=0;     
   ///}
 
}

// used when recieving sensor values from the gogoboard
void fillBlankSensorsWithDefaultValue(void) {
   int i;
   
   for (i=0;i<8;i++) {
      // if the sensor value has not been updated
      // then fill in the values with 1023
      if (!bit_test(gblUpdatedSensors, i)) {   
         gblSensorValues[i]=1023;            
      }
   }

}



void clearScreen() {
  inputCursor=0;
  strcpy(curText,"                                ");
  gblDirtyBits=0xffffffff;  // set all 32 bits as dirty
  triggerScreenUpdate();
}

void showError(int errCode, int data) {
   if (!DEBUG_ON)  return;
   
   // todo: show error message on the screen
   clearScreen();
   sprintf(curText, "Error #%u, %u", errCode, data);
   gblDirtyBits=0xffffffff;  // set all 32 bits as dirty
   triggerScreenUpdate();
}

void showCursor(){
   output_low(PIN_RS);
   output_b(0x0F);
   submit();
}

void hideCursor(){
   output_low(PIN_RS);
   output_b(0x0C);
   submit();
}


void setPosition(int pos){
   output_low(PIN_RS);
   output_b(0x80 + 0x40 * !!( pos & 0x10 ) + ( pos & 0x0F ));
   submit();
   gblDisplayModuleCursorPos = pos;   // update variable that tracks the display cursor
}

int getPosition() {
   return(gblDisplayModuleCursorPos);
}

void twoDisplay(){
   output_b(0x38);
   submit();
}

void init(){
   output_low(PIN_C2);   // debug pin

   delay_ms(100);
   output_b(0x00);
   output_low(PIN_EN);
   output_low(PIN_RW);
   output_low(PIN_RS);
   hideCursor();
   twoDisplay();

   //type('S');type('t');type('a');type('r');type('t');type('i');type('n');type('g');type('.');type('.');type('.');
   //delay_ms(1500);


   setPosition(0);
   triggerScreenUpdate();

   setup_adc_ports(NO_ANALOGS);
   setup_adc(ADC_OFF);
   setup_counters(RTCC_INTERNAL,RTCC_DIV_256);
   setup_timer_1(T1_INTERNAL|T1_DIV_BY_1);

   set_timer1(T1_COUNTER);

   
   enable_interrupts(INT_SSP);
   enable_interrupts(GLOBAL);   

   resetI2C();    // clear the i2c circuity


}


// update 1 character on the screen. 

void updateScreen() {

   
   if(gblDisplayBufferIndex==32){  // we have updated all the characters
      disable_interrupts(INT_TIMER1);
   }
   else {

      // if current char has changed -> display it
      if (bit_test(gblDirtyBits, gblDisplayBufferIndex)) {
         bit_clear(gblDirtyBits, gblDisplayBufferIndex);
         
         // if the display's screen cursor does not match the buffer index
         // then we have to update the cursor position.
         if (gblDisplayBufferIndex != getPosition()) {
            setPosition(gblDisplayBufferIndex);
         }

         if(gblDisplayBufferIndex==16){  // If at second line -> we must manually set the position
           setPosition(16);
         }

         type(curText[gblDisplayBufferIndex]);
      } 
      gblDisplayBufferIndex++;
   }
}

void show_temp(int16 temp){
//!   if (gblTimeToUpdateScreen) {
//!         gblTimeToUpdateScreen = 0;
//!         updateScreen();
//!         }
   setPosition(0);
   dis0 =  ((temp/1000)+48);
   dis1 =  (((temp%1000)/100)+48);
   disf1 = ((((temp%1000)%100)/10)+48);
   disf2 = ((((temp%1000)%100)%10)+48);
   type('T');type('e');type('m');type('p');type(':');
   type(dis0);type(dis1);type('.');type(disf1);type(disf2);type('\0');type('C');
   
}

void show_humid(int16 humid){
//!   if (gblTimeToUpdateScreen) {
//!         gblTimeToUpdateScreen = 0;
//!         updateScreen();
//!         }
   setPosition(16);
   dis0 =  ((humid/1000)+48);
   dis1 =  (((humid%1000)/100)+48);
   disf1 = ((((humid%1000)%100)/10)+48);
   disf2 = ((((humid%1000)%100)%10)+48);
   type('H');type('u');type('m');type('i');type('d');type(':');
   type(dis0);type(dis1);type('.');type(disf1);type(disf2);type('\0');type('%');
   
}

void show_water(int16 water){
      water=water/16;
//!      if (gblTimeToUpdateScreen) {
//!         gblTimeToUpdateScreen = 0;
//!         updateScreen();
//!         }
      setPosition(15);type('\0');
  
      type('W'); type('a'); type('t'); type('e'); type('r'); type(':');

      char dis0=(water/10000)+48;
      char dis1=((water%10000)/1000)+48;
      char dis2=(((water%10000)%1000)/100)+48;
      char dis3=((((water%10000)%1000)%100)/10)+48;
      char dis4=((((water%10000)%1000)%100)%10)+48;
      
      dis0 = dis0 == '0' ? '\0' : dis0;
      type(dis0); type(dis1); type(dis2); type(dis3); type(dis4); type('\0'); type('c'); type('m');
}

void show_sonic(int16 sonic){
      sonic=sonic/16;
//!      if (gblTimeToUpdateScreen) {
//!         gblTimeToUpdateScreen = 0;
//!         updateScreen();
//!         }
      sonic=600-sonic;
      setPosition(15);type('\0');
  
      type('W'); type('a'); type('t'); type('e'); type('r'); type(':');

      char dis0=(sonic/10000)+48;
      char dis1=((sonic%10000)/1000)+48;
      char dis2=(((sonic%10000)%1000)/100)+48;
      char dis3=((((sonic%10000)%1000)%100)/10)+48;
      char dis4=((((sonic%10000)%1000)%100)%10)+48;
      
      dis0 = dis0 == '0' ? '\0' : dis0;
      type(dis0); type(dis1); type(dis2); type(dis3); type(dis4); type('\0'); type('c'); type('m');
}


void show_counter(int count){
      setPosition(31);
      type('\0');
   
      type('C');type('o');type('u');type('n');type('t');type('=');type('\0');
   
      char dis0=(count/10000)+48;
      char dis1=((count%10000)/1000)+48;
      char dis2=(((count%10000)%1000)/100)+48;
      char dis3=((((count%10000)%1000)%100)/10)+48;
      char dis4=((((count%10000)%1000)%100)%10)+48;
      
      dis0 = dis0 == '0' ? '\0' : dis0;
      type(dis0); type(dis1); type(dis2); type(dis3); type(dis4);
   
}
// val_adc is value of analog ot digital from mcp3208
// line is position of LCD display
// ch is channel of ADC
void show_adc(int16 val_adc,int line,int ch){
      
      val_adc=val_adc/16;
         
      if(line==1)
         setPosition(0);
      else if(line==2)
         setPosition(16);
      else if(line==3){
         setPosition(15);
         type('\0');}
       else if(line==4){
         setPosition(31);
         type('\0');}
         
       
         
      type('A');
      type('D');
      type('C');
      type(ch+48);
      type('\0');
      type('=');
      type('\0');
      
      
      
//!      unsigned char dis1=(val_adc/1000)+48;
//!      unsigned char dis2=((val_adc%1000)/100)+48;
//!      unsigned char dis3=(((val_adc%1000)%100)/10)+48;
//!      unsigned char dis4=(((val_adc%1000)%100)%10)+48;
      
      
      char dis0=(val_adc/10000)+48;
      char dis1=((val_adc%10000)/1000)+48;
      char dis2=(((val_adc%10000)%1000)/100)+48;
      char dis3=((((val_adc%10000)%1000)%100)/10)+48;
      char dis4=((((val_adc%10000)%1000)%100)%10)+48;
      
      
      type(dis0);
      type(dis1);
      type(dis2);
      type(dis3);
      type(dis4);
}

//!void main(void) {
//!   set_tris_c(0b00011001);
//!   init();
//!   
//!   
//!   while(1){
//!      if (gblTimeToUpdateScreen) {
//!         gblTimeToUpdateScreen = 0;
//!         updateScreen();
//!      }
//!   }
//!   int16 val_adc=0;
//!   int ch[8]={0,1,2,3,4,5,6,7};
//!   set_tris_c(0b00011001);
//!   setup_adc(adc_off);
//!   
//!   output_bit(PIN_A3,0);
//!   output_bit(PIN_A4,0);
//!   
//!   
//!   init();  //LCD Init
//!   adc_init();
//!   int sw1=0;
//!   int sw2=0;
//!   int i=0;
//!   
//!   while(1){
//!   
//!   if (gblTimeToUpdateScreen) {
//!         gblTimeToUpdateScreen = 0;
//!         updateScreen();
//!      }
//!      
//!      setPosition(0);
//!      type('I');
//!      type('/');
//!      type('O');
//!      type(' ');
//!      type('M');
//!      type('o');
//!      type('d');
//!      type('u');
//!      type('l');
//!      type('e');
//!      type('\0');
//!   
//!      
//!   val_adc=read_analog(ch[0]);
//!   show_adc(val_adc,2,ch[0]);
//!   
//!   val_adc=read_analog(ch[1]);
//!   show_adc(val_adc,3,ch[1]);
//!   
//!   val_adc=read_analog(ch[2]);
//!   show_adc(val_adc,4,ch[2]);
//!   
//!   show_counter(i);
//!   sw1= input(PIN_A0);
//!   if(sw1==0){
//!      
//!      output_high(PIN_A3);
//!      delay_ms(150);
//!      i=i+1;
//!      if(i==100)i=0;
//!   }
//!   else {
//!      output_low(PIN_A3);
//!      delay_ms(20);}
//!   
//!   //Switch Relay
//!   sw2=input(PIN_A1);
//!   if(sw2==0){
//!      output_high(PIN_A4);
//!      delay_ms(250);}
//!   else{
//!      output_low(PIN_A4);
//!      delay_ms(20);}
//!   }

//! }


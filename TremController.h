#ifndef _USE_TREM_CONTROLLER_h
#define _USE_TREM_CONTROLLER_h

////////////////////////////////////////////////
/////////PIN NUMBER DEFINITIONS
////////////////////////////////////////////////
//interrupt pins
#define  INTERRUPT_PIN_0                  2
#define  INTERRUPT_PIN_1                  3
//lcd pins
#define  LCD_D7_PIN                       7 
#define  LCD_D6_PIN                       6
#define  LCD_D5_PIN                       5
#define  LCD_D4_PIN                       4
#define  LCD_EN_PIN                       8
#define  LCD_RS_PIN                       9
//trem pulse selector
#define  TREM_PULSE_SEL                   10
//encoder pins 
#define  TREM_TIME_BASE_ENCODER_PIN       12  
//trem on/off control output pin for device
#define  TREM_STATE_PIN                   13 
//tap tempo switch
#define  TAP_TEMPO_SW_PIN                 11
#define  TAP_TEMPO_LED_OUTPUT             14

////////////////////////////////////////////////
///////////Trem Pulse & Time definitions////////
////////////////////////////////////////////////
#define MAX_PULSES           8
#define ACTUAL_PULSES        8    
#define MAX_TIME_SCALE       10     //multiplier for max trem loop
#define MAX_ENCODER_READING  10000    //upper limit for encoder input value
#define MIN_ENCODER_READING  0      //lower limit for encoder input value
#define ENCODER_POLL         10    //num inputs to encoder pins before reading the value
#define ENC_POLL_TIMEOUT     500
#define ENC_WAIT_TO_NEXT_SET 50
#define INT0 0 //interrupt0
#define INT1 1 //interrupt1
#define NUM_ENCODER_PINS     1   //just one encoder used, for the pulse time base

////////////////////////////////////////////////
///////////Other definitions////////
////////////////////////////////////////////////
#define TOGGLE               1 ^ //XOR


//Globals

////////////////////////////////////////////////
///////Default Pulse time table/////////////////
////////////////////////////////////////////////
int pulseDef[ACTUAL_PULSES][3] = 
{
//ON    OFF   EN
  100, 101, 1,  //pulse01
  200, 202, 1,  //pulse02
  300, 303, 0,  //pulse03
  400, 404, 0,  //pulse04
  500, 505, 0,  //pulse05
  600, 606, 0,  //pulse06
  700, 707, 0,  //pulse07
  800, 808, 0,  //pulse08
}; //TO DO: read defaults from a file, add ability to save settings


#endif //_USE_TREM_CONTROLLER_h

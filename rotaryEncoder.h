#ifndef _ROTARY_ENC_h
#define  _ROTARY_ENC_h

/******************************************
Includes  ////////////////////////////////
******************************************/
#include <Arduino.h>
#include "vector.h"

////////////////////////////////////////////////
/////////definitions
////////////////////////////////////////////////
#define MULTIPLIER_TIMEOUT 100 //milliseconds
#define ENC_POLL_TIMEOUT     500
#define LONG_PRESS_TIMER   2000
#define ENC_BASE_INCREMENT 1
////////////////////////////////////////////////
/////////GLOBALS
////////////////////////////////////////////////
static int A_SIG=0, B_SIG=1;
static long milliA_Fall = 0,milliB_Fall = 0, milliA_Rise = 0, milliB_Rise = 0;
static long multiplierTimer = 0;
static boolean aRiseDone = false, aFallDone = false, bRiseDone = false, bFallDone = false;

////encoder device structure
//typedef struct _encoderDevice{
//  uint8_t   pinNumber;  // encoder pin number on the board
//  uint8_t   pinState;  //state of the encoder's pin
//  uint16_t  count;      //running total of the pulses 
//} EncoderDevice;


class EncoderDevice
{
  public:
  EncoderDevice():pinNumber(0),pinState(0),value(0),deviceType(0){};//constructor
    int pinNumber;
    int value;
    int deviceType; //0 = rotary, 1 = switch
    int encMinVal;
    int encMaxVal;
    int encDefaultVal;
  private:
    int pinState;
};


class EncoderBus
{
  public:
    EncoderBus():_idx(0){}; //constructor
    void initialize(int interruptPin1, int interruptPin2);
    void setupPin(int encPinNum, int encDevType, int encMinVal, int encMaxVal);
    int readEncoderBus(int pinNum);
    void resetEncValue(int pinNum, int inValue);
    void A_FALL(); //has to be public to allow attachinterrupt() call from a method (using wrapper functions). But dont use outside of library!!!
    void A_RISE();
    void B_FALL();
    void B_RISE();
  private:
    int getEncoderPinStates(); 
    void updateEncVecs();
    void clearEncVecs();
    void checkEncoderBusState(int encChgIdx);
    int clamp(int checkVal, int listIdx);
    int _idx;
    int _interruptPin1, _interruptPin2, _encPinNum, _encId;
    Vector<int> _pinNumberVec, _pinStateVec, _pinValueVec, _deviceTypeVec, _deviceMinVal, _deviceMaxVal; //list for accessing the pin numbers and types in the library 
    Vector<EncoderBus *> _encBusCpy;
};


#endif  //#ifndef _ROTARY_ENC_h







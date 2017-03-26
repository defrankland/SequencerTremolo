//Encoder library for the arduino uno board (interrupt pins 0 and 1)
//Each encoder is connected to the Up and Down interrupts,
//then connect only the UP pin for each encoder.
//That way only one pin is needed for each new encoder
 

 #include <Arduino.h>
 #include "rotaryEncoder.h"
 
/*globals*/
static int _tableRow = 0;
EncoderBus encBusGbl;
int ffMultiplier = 1;

 /*****************************************************************************************
 prototypes for static functions
 *****************************************************************************************/
 extern void _A_FALL();
 extern void _A_RISE();
 extern void _B_FALL();
 extern void _B_RISE();
 static boolean checkEncTimeout();
 
 /***********************************************************
 * Encoder class functions
 ***********************************************************/
 
 /*initialize() - set the interrupt pins*/
 void EncoderBus::initialize(int interruptPin1, int interruptPin2)
 {
   attachInterrupt(interruptPin1, _A_FALL, FALLING);
   attachInterrupt(interruptPin2, _B_FALL, FALLING);
 }
 
 /*setPin() - sets the pin to use for each encoder ******************
 *
 * args: the pin number and the device type (0 = rotary, 1 = switch)
 ********************************************************************/
 void EncoderBus::setupPin(int encPinNum, int encDevType, int encMinVal, int encMaxVal)
 {
   pinMode(encPinNum, INPUT);
   
   //as encoder devices are added to the encoder bus, 
   //add them to these vectors for tracking
   _pinNumberVec.push_back((size_t) encPinNum);  //pin number 
   if(encDevType == 0)
   {
     _pinStateVec.push_back(0);                    //initialize pin state to zero 
     _pinValueVec.push_back(0);                    ///initialize value to zero 
   }
   else if(encDevType == 1)
   {
     _pinStateVec.push_back(-1);                    //initialize pin state 
     _pinValueVec.push_back(digitalRead(encPinNum));                    ///initialize value to zero 
   }
   _deviceTypeVec.push_back(encDevType);         //encoder type
   _deviceMinVal.push_back(encMinVal);           //min value
   _deviceMaxVal.push_back(encMaxVal);           //max value
 }
 
 /*****************************************************************************************
 method EncoderBus::readEncoderBus() - returns the value for the encoder
                                       tied to a pin number 
 *****************************************************************************************/
 int EncoderBus::readEncoderBus(int pinNum)
 {
   int encLstIdx = 0;
   int encVal = 0;
   boolean pinFound = false;
   
   for(encLstIdx; encLstIdx < _pinNumberVec.size(); encLstIdx++)
   {
       if(_pinNumberVec[encLstIdx] == pinNum)
       { 
          encVal = _pinValueVec[encLstIdx];
          pinFound = true;
       }
   }
   return encVal;
 }
 
 
/*****************************************************************************************
 method EncoderBus::resetEncValue() - sets the encoder at the passed pin number
                                       to the passed value
 *****************************************************************************************/
 void EncoderBus::resetEncValue(int pinNum, int inValue)
 {
   int encLstIdx = 0;
   boolean pinFound = false;
   
   for(encLstIdx; encLstIdx < _pinNumberVec.size(); encLstIdx++)
   {
       if(_pinNumberVec[encLstIdx] == pinNum)
       { 
          _pinValueVec[encLstIdx] = inValue;
          pinFound = true;
       }
   }
 }
 
  /*****************************************************************************************
 method EncoderBus::getEncoderPinStates() - private method called by the interrupt state 
                                            change functions. Gets the pin status of all
                                            pins added to the EncoderBus
 *****************************************************************************************/
 int EncoderBus::getEncoderPinStates()
 {
   int encLstIdx = 0;
   int retVal = 0;
   //NOTE: Added delay when working with 2 encoders, the first encoder in the table was not changing state fast enough
   //before reading but the second read ok. Probably need to adjust capacitor values.
   delay(15);

   for(encLstIdx; encLstIdx < _pinNumberVec.size(); encLstIdx++)
   {
     if(_deviceTypeVec[encLstIdx] == 0){_pinStateVec[encLstIdx] += digitalRead(_pinNumberVec[encLstIdx]);}
     if((_deviceTypeVec[encLstIdx] == 1) && (_pinStateVec[encLstIdx] != digitalRead(_pinNumberVec[encLstIdx])))
     {
       _pinStateVec[encLstIdx] = digitalRead(_pinNumberVec[encLstIdx]);
       retVal = 1;
     }
   }
   return retVal;
 }
 
/**********************************************************************************************
      A|B      A|B                         A_FALL, B_FALL, A_RISE, B_RISE ===> increase
      ===      ===     Order of events:                   ~~~OR~~~~
      0|1      1|0                         B_FALL, A_FALL, B_RISE, A_RISE ===> decrease
      0|0      0|0
      1|0      0|1
      1|1      1|1 
**********************************************************************************************/
void EncoderBus::A_FALL()
{
  int encIdx = 0;
  detachInterrupt(INT0); //detach pin 1
  A_SIG=0;
  aFallDone=true;
  aRiseDone = false;
  
  milliA_Fall = millis();
  encIdx = getEncoderPinStates();
  
  attachInterrupt(INT0, _A_RISE, RISING);
  
  checkEncoderBusState(encIdx);
}

void EncoderBus::A_RISE()
{
  int encIdx = 0;
  detachInterrupt(INT0); //detach pin 1
 
  A_SIG=1;
  aRiseDone = true;
  
  milliA_Rise = millis();
  encIdx = getEncoderPinStates();
  
  attachInterrupt(INT0, _A_FALL, FALLING);
  
  checkEncoderBusState(encIdx);
}

void EncoderBus::B_FALL()
{
  int encIdx = 0;
  detachInterrupt(INT1); //detach pin 2
 
  B_SIG=0;
  bFallDone = true;
  bRiseDone = false;
  
  milliB_Fall = millis();
  encIdx = getEncoderPinStates();
  
  attachInterrupt(INT1, _B_RISE, RISING);
  
  checkEncoderBusState(encIdx);
}

void EncoderBus::B_RISE()
{
  int encIdx = 0;
  detachInterrupt(INT1); //detach pin 2

  B_SIG=1;
  
  milliB_Rise = millis();
  encIdx = getEncoderPinStates();
  
  bRiseDone = true;
  attachInterrupt(INT1, _B_FALL, FALLING);
  
  checkEncoderBusState(encIdx);
}


void EncoderBus::updateEncVecs()
{
  int changeVal = ENC_BASE_INCREMENT; 
  int i = 0;
  
  if (millis() - multiplierTimer < MULTIPLIER_TIMEOUT){ffMultiplier = ffMultiplier * 2;}
  else{ffMultiplier = 1;}
  
  for(i; i<_pinNumberVec.size(); i++)
  {
    if(_pinStateVec[i] == 2 && _deviceTypeVec[i] == 0) //The active rotary encoder
    {
      //incrementing
      if (milliA_Fall < milliB_Fall && milliB_Fall< milliA_Rise && milliA_Rise < milliB_Rise)
      {
        _pinValueVec[i] = clamp(_pinValueVec[i] + (changeVal * ffMultiplier), i); 
      }
      //decrementing
      if (milliB_Fall < milliA_Fall && milliA_Fall < milliB_Rise && milliB_Rise < milliA_Rise)
      {
        _pinValueVec[i] = clamp(_pinValueVec[i] - (changeVal * ffMultiplier), i); 
      }
    }
    else if(_deviceTypeVec[i] == 1)//The active switch
    {
      if (_pinStateVec[i] == 0)
      {
        if(_pinValueVec[i] < 14){_pinValueVec[i]++;}
        else                   {_pinValueVec[i] = 0;}
      }
//      Serial.print(_deviceTypeVec[i]);
//      Serial.print(" ");
//      Serial.print(_pinStateVec[i]);
//      Serial.print(" ");
//      Serial.print(_pinValueVec[i]);
//      Serial.print("\n");
    }
  }
  //Reset the time captures for the interrupt events   
  milliA_Fall = 0;
  milliB_Fall = 0;
  milliA_Rise = 0;
  milliB_Rise = 0;
  
  multiplierTimer = millis();
}

void EncoderBus::clearEncVecs()
{
  int i = 0;
  
  for(i; i<_pinNumberVec.size(); i++)
  {
    if(_deviceTypeVec[i] == 0)
    {
      _pinStateVec[i] = 0;  //reset encoder state pin
    }
    else if(_deviceTypeVec[i] == 1)
    {
       _pinStateVec[i] = 1;  //reset switch state pin
    }
  }
}

void EncoderBus::checkEncoderBusState(int encChgIdx)
{
    if((aRiseDone == true && aFallDone == true && bRiseDone == true && bFallDone == true)  || 
      (encChgIdx == 1 && aFallDone == true) || //for a switch device. Will function on button down
      (encChgIdx == 1 && bFallDone == true) || 
      (checkEncTimeout()))
    {
      aRiseDone = false; aFallDone = false; bRiseDone = false; bFallDone = false;
      
      updateEncVecs(); //check if state changes received
      clearEncVecs(); //reset table after reading
    }
}

/************************************************************
clamp() - returns the value to set the encoder device to.
     1. Returns the passed velue if it's ok to set
     2. Checks if the difference from the max or min value 
        is within range, then sets it to the min or max
************************************************************/

int EncoderBus::clamp(int checkVal, int listIdx)
{
  int retVal = 0;
  int minValDiff = checkVal - _deviceMinVal[listIdx]; 
  //int maxValDiff = checkVal - _deviceMaxVal[listIdx];
  
  if(checkVal >= _deviceMinVal[listIdx] && checkVal <= _deviceMaxVal[listIdx]){retVal = checkVal;}  //set to passed value
  else if(minValDiff <= 0){retVal = _deviceMinVal[listIdx];}                                       //closer to min value
  else {retVal = _deviceMaxVal[listIdx];}                                                          //closer to max value
  
  return retVal;
}

/**********************************************************************************************************
checkEncTimeout() - flag to clear state pins if timeout is exceeded 
**********************************************************************************************************/
boolean checkEncTimeout()
{
  long timers[4] = {milliA_Fall, milliB_Fall, milliA_Rise, milliB_Rise};
  long latestTimer = milliA_Fall;
  int idx = 0; 
  boolean retVal = false;
  for(idx; idx < 4 - 1 ; idx++)
     if(timers[idx] > latestTimer){latestTimer = timers[idx];}
  if (millis() - latestTimer > ENC_POLL_TIMEOUT){retVal = true;}
  
  return retVal;
}




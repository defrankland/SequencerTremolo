/*

v0_8 -- 

DESCRIPTION:
  _   _   _   _   _   _   _   _
_| |_| |_| |_| |_| |_| |_| |_| |  ---> back to pulse numer 1

1. total of 8 pulses, each with its own time base
2. program time base by selecting the pulse number, then on time / off time
3. one encoder is used to set the on/off time for any selected pulse
4. can set the number of pulses to use, between 1 and MAX_PULSES

*/

//TO ADD:
#include <LiquidCrystal.h>
#include "rotaryEncoder.h"
#include "TremController.h"
#include "classDefs.h" 
#include <EEPROM.h>


//globals
EncoderBus EncoderBus; //create instance of the EncoderBus class - will handle all encoders
TremPulseDev tremTimeBaseEncoder;
TremSwDev tremPulseSelectSwitch;
TapTempoSw tapTempoSw;
int pulseSelSwReadingLast;
int tapTempoSwReadingLast;
int currPulseNum;
int currCol;
long outputTimer;
long tapTempoStartTime;

//prototypes
void encoderInit();
void checkLongPress();

////////////////////////////////////////////////
// initialize the lcd pins//////////////////////
////////////////////////////////////////////////
LiquidCrystal lcd(LCD_RS_PIN, LCD_EN_PIN, LCD_D4_PIN, 
                  LCD_D5_PIN, LCD_D6_PIN, LCD_D7_PIN);


////////////////////////////////////////////////
/////////Setup()////////////////////////////////
////////////////////////////////////////////////
void setup()
{ 
  //initialize pins
  pinMode(TREM_STATE_PIN, OUTPUT); //time control pin to device
  pinMode(TAP_TEMPO_LED_OUTPUT, OUTPUT); //tempo led pin
  //pinMode(TREM_PULSE_SEL, INPUT);

  //initialize encoders
  encoderInit();
  
  //initialize tap tempo
  tapTempoInit();
  
  // initialize the lcd
  lcd.begin(16, 2); //16 columns, 2 rows
  startupLCD();
  
  //start the PC-to-arduino port - debug output
  Serial.begin(115200);
  
  currPulseNum = 0;
  currCol = 0;
  outputTimer = millis();
}

////////////////////////////////////////////////
/////////loop()/////////////////////////////////
////////////////////////////////////////////////
void loop()
{
  //timerScaling();  //check if the time scale knob has changed
  //setPulseTimer(); //get the trem event time
  //setTremState(); //check if trems state needs to change
  
  tremUpdate();
  lcdUpdate();
  checkLongPress();
  calculateBpm();
  
  setTremState();
  tapTempoBLink();
}

void encoderInit()
{
  //start encoder interrupt pins
  EncoderBus.initialize(INT0, INT1);
   
  //initialize trem time base rotary encoder device
  tremTimeBaseEncoder.pinNumber = TREM_TIME_BASE_ENCODER_PIN;
  tremTimeBaseEncoder.deviceType = 0;
  tremTimeBaseEncoder.encMinVal = MIN_ENCODER_READING;
  tremTimeBaseEncoder.encMaxVal = MAX_ENCODER_READING;
  tremTimeBaseEncoder.numberOfPulses = 8;
   
  //pulse time selector switch init 
  tremPulseSelectSwitch.pinNumber = TREM_PULSE_SEL;
  tremPulseSelectSwitch.deviceType = 1;
  tremPulseSelectSwitch.encMinVal = 0;
  tremPulseSelectSwitch.encMaxVal = ACTUAL_PULSES;
  
  //tap tempo switch init
  tapTempoSw.pinNumber = TAP_TEMPO_SW_PIN;
  tapTempoSw.deviceType = 1;
  tremPulseSelectSwitch.encMinVal = 0;
  tremPulseSelectSwitch.encMaxVal = 1;
  
  //add the encoder devices to the encoder bus
  EncoderBus.setupPin(tremPulseSelectSwitch.pinNumber, tremPulseSelectSwitch.deviceType, tremPulseSelectSwitch.encMinVal, tremPulseSelectSwitch.encMaxVal);
  EncoderBus.setupPin(tremTimeBaseEncoder.pinNumber, tremTimeBaseEncoder.deviceType, tremTimeBaseEncoder.encMinVal, tremTimeBaseEncoder.encMaxVal);
  EncoderBus.setupPin(tapTempoSw.pinNumber, tapTempoSw.deviceType, tapTempoSw.encMinVal, tapTempoSw.encMaxVal);
  
  //setup the trem times and pulses from defaults
  for(int i = 0; i < tremTimeBaseEncoder.numberOfPulses; i++)
  {                                                               
    tremTimeBaseEncoder.pulseOnTimeVec.push_back(pulseDef[i][0]);   ////ON Time(ms)   
    tremTimeBaseEncoder.pulseOffTimeVec.push_back(pulseDef[i][1]);   //OFF Time(ms)      
    tremTimeBaseEncoder.pulseEnabledVec.push_back(pulseDef[i][2]);    //enabled
  }
    
  //initilaize to the first pulse
  for(int j = 0; j < tremTimeBaseEncoder.pulseOnTimeVec.size(); j++)
  {
    if(tremTimeBaseEncoder.pulseEnabledVec[j] == 1)
    {
      tremTimeBaseEncoder.selectedPulseNumber = j; //set the first pulse time
      tremTimeBaseEncoder.selectedPulsePart = 0; //start with ON part selected
      EncoderBus.resetEncValue(tremTimeBaseEncoder.pinNumber, tremTimeBaseEncoder.pulseOffTimeVec[j]);//set the encoder value to the default value for the 1st enabled pulse
      break;
    }
    else
    {
      if(j == 8) {Serial.print("ERROR: No pulses configured as enabled. check configuration file.");}
    }
  }
}



/*********************************************************************************************************
tremUpdate() - 
          1. reset the trem pulse to the encoder reading if it has changed
          2. check if switch has been pressed
*********************************************************************************************************/
void tremUpdate()
{
  //1. reset the trem pulse to the encoder reading if it has changed
  if(tremTimeBaseEncoder.selectedPulsePart == 0)//ON part
  {
    tremTimeBaseEncoder.pulseOnTimeVec[tremTimeBaseEncoder.selectedPulseNumber] = EncoderBus.readEncoderBus(tremTimeBaseEncoder.pinNumber);
  }
  if(tremTimeBaseEncoder.selectedPulsePart == 1)//OFF part
  {
    tremTimeBaseEncoder.pulseOffTimeVec[tremTimeBaseEncoder.selectedPulseNumber] = EncoderBus.readEncoderBus(tremTimeBaseEncoder.pinNumber);
  }
  
  //2. check if switch has been pressed
  if(pulseSelSwReadingLast != (EncoderBus.readEncoderBus(tremPulseSelectSwitch.pinNumber) - 1))
  {
    if(tremTimeBaseEncoder.selectedPulsePart == 0) //If switch pressed, change from ON part selected to OFF part selected
    {
      //Serial.print(("Old part: "); Serial.print(tremTimeBaseEncoder.selectedPulseNumber); Serial.print(", ");
      tremTimeBaseEncoder.selectedPulsePart = 1;
      EncoderBus.resetEncValue(tremTimeBaseEncoder.pinNumber, tremTimeBaseEncoder.pulseOffTimeVec[tremTimeBaseEncoder.selectedPulseNumber]); //new pulse time selected, reset the encoder reading to match the table. 
      //Serial.print(("New part: "); Serial.print(tremTimeBaseEncoder.selectedPulseNumber); Serial.print("\n");
    }
    else //If switch pressed and OFF part selected, change to the next pulse or roll back to pulse 0
    {
      //Serial.print(("Old num: "); Serial.print(tremTimeBaseEncoder.selectedPulseNumber); Serial.print(", ");
      tremTimeBaseEncoder.selectedPulsePart = 0;
      if(tremTimeBaseEncoder.selectedPulseNumber <  tremTimeBaseEncoder.numberOfPulses - 1){tremTimeBaseEncoder.selectedPulseNumber++;}
      else {tremTimeBaseEncoder.selectedPulseNumber = 0;}
      EncoderBus.resetEncValue(tremTimeBaseEncoder.pinNumber, tremTimeBaseEncoder.pulseOnTimeVec[tremTimeBaseEncoder.selectedPulseNumber]); //new pulse time selected, reset the encoder reading to match the table.
    }
    
    pulseSelSwReadingLast = (EncoderBus.readEncoderBus(tremPulseSelectSwitch.pinNumber) - 1);
    tremPulseSelectSwitch.lastPressTime = millis();
    tremPulseSelectSwitch.longPressDoneFlag = false;
    tremPulseSelectSwitch.longPressCheckState = digitalRead(TREM_PULSE_SEL);
  }
}

void lcdUpdate()
{
  //lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("P");
  lcd.print(tremTimeBaseEncoder.selectedPulseNumber);
  lcd.print("_H=");
  lcd.print(tremTimeBaseEncoder.pulseOnTimeVec[tremTimeBaseEncoder.selectedPulseNumber], DEC);
  lcd.print("ms");
  if(tremTimeBaseEncoder.selectedPulsePart == 0){lcd.print("<-");}
  lcdClearLnEnd();
  if(tremTimeBaseEncoder.pulseEnabledVec[tremTimeBaseEncoder.selectedPulseNumber] == false)
  {
    lcd.setCursor(15,0);
    lcd.print("X");
  }
  
  lcd.setCursor(0, 1);
  lcd.print("P");
  lcd.print(tremTimeBaseEncoder.selectedPulseNumber);
  lcd.print("_L=");
  lcd.print(tremTimeBaseEncoder.pulseOffTimeVec[tremTimeBaseEncoder.selectedPulseNumber], DEC);
  lcd.print("ms");
  if(tremTimeBaseEncoder.selectedPulsePart == 1){lcd.print("<-");}
  lcdClearLnEnd();
  if(tremTimeBaseEncoder.pulseEnabledVec[tremTimeBaseEncoder.selectedPulseNumber] == false)
  {
    lcd.setCursor(15,1);
    lcd.print("X");
  }
}

//the crappy way to do it...
void lcdClearLnEnd()
{
    for(int r = 0; r < 15; r++)
    {
      lcd.print(" ");
      
    }
}


void checkLongPress()
{ 
  if(digitalRead(TREM_PULSE_SEL) != tremPulseSelectSwitch.longPressCheckState)
 {
  // Serial.print(tremPulseSelectSwitch.longPressCheckState);
   //Serial.print("\n");
   tremPulseSelectSwitch.longPressDoneFlag = true;
 } 
  
  if((digitalRead(TREM_PULSE_SEL) == tremPulseSelectSwitch.longPressCheckState)                &&
      (millis() - tremPulseSelectSwitch.lastPressTime > tremPulseSelectSwitch.longPressTime)   &&
      (tremPulseSelectSwitch.longPressDoneFlag == false))
  {
    Serial.print(" disable/enable\n");
    tremTimeBaseEncoder.pulseEnabledVec[tremTimeBaseEncoder.selectedPulseNumber] = TOGGLE tremTimeBaseEncoder.pulseEnabledVec[tremTimeBaseEncoder.selectedPulseNumber]; 
    tremPulseSelectSwitch.longPressDoneFlag = true;
  }
  
}
/*******************************************************************
* startupLCD() - display when powered up
********************************************************************/
void startupLCD()
{
  // Print a message to the LCD.
  lcd.print("Sequencer Trem");
  lcd.setCursor(0, 1);
  lcd.print("v0.x (beta)");
  
  
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("****************");
  lcd.setCursor(0, 1);
  lcd.print("****************");
  delay(500);
  lcd.clear();
}

/*******************************************************************
* setTremState() - Adjust the output pin if time has elapsed
********************************************************************/
void setTremState()
{   
  int timeToCheck = 0;
  int startPulseNum = currPulseNum;
  boolean okToSetTrem = true;
  
  while (!tremTimeBaseEncoder.pulseEnabledVec[currPulseNum])
  {
    if(currPulseNum < 7){currPulseNum++;}
    else currPulseNum = 0;
    if(startPulseNum == currPulseNum) //no pulses enabled
    {
      //Serial.print("ERROR: No pulses enabled\n");
      okToSetTrem = false;
    }
  }
  
  //check if ON time or OFF time is the current pulse part
  if(currCol == 0){
    timeToCheck = tremTimeBaseEncoder.pulseOnTimeVec[currPulseNum];}
  else //currCol == 0
    timeToCheck = tremTimeBaseEncoder.pulseOffTimeVec[currPulseNum];
    
  if( currPulseNum < ACTUAL_PULSES && okToSetTrem)
  {
      if (timeToCheck <= (millis() - outputTimer))
      { 
       if(!timeToCheck ==0) //don't do anything if time = 0ms
       { 
        if(currCol == 0)
        {
          digitalWrite(TREM_STATE_PIN, LOW); 
        }
        else
        {
          digitalWrite(TREM_STATE_PIN, HIGH);
        }
       }
        
        outputTimer = millis();
   
        if(currCol == 0)
        {
          currCol++;
        }
        else
        {
          currCol = 0;
          currPulseNum++;
          if(currPulseNum == ACTUAL_PULSES)  //reset the trem event loop
          {
            //pulseTimer = 0;
            currPulseNum = 0;
          }
        }
  
      }
  }
  else
  {
    //fix it
  }
}


void tapTempoInit()
{
  tapTempoSw.tapBpm = 120;
  tapTempoSw.tapTempoMeasureTime = 8000;
  tapTempoSw.tapTempoTime = 1000;
  
}


/*******************************************************************
* calculateBpm() - calculate bpm
      1. get new tap tempo time if changed
      2. calculate the bpm
      3. calculate the measure time
********************************************************************/
void calculateBpm()
{
   boolean tapTempoChanged = false;
   int bpmTimeLast = tapTempoSw.tapTempoTime;
   int measureTimeLast = tapTempoSw.tapTempoMeasureTime;
  
   //1. get new tap tempo time if changed
   if(tapTempoSwReadingLast != EncoderBus.readEncoderBus(tapTempoSw.pinNumber) - 1 )//check if tap tempo button has been pressed
   {
     if(millis() - tapTempoSw.tapTempoSwTimePress < tapTempoSw.tapTempoTimeout)
     {
       tapTempoSw.tapTempoTime = (tapTempoSw.tapTempoTime + (millis() - tapTempoSw.tapTempoSwTimePress)) / 2; //re-calculate the tapTempoTime
       tapTempoChanged = true;
     }
     tapTempoSw.tapTempoSwTimePress = millis(); //update time of last press
     tapTempoSwReadingLast = (EncoderBus.readEncoderBus(tapTempoSw.pinNumber) - 1);
   }
   
   if(tapTempoChanged)
   { 
     ///if(abs(tapTempoSw.tapTempoTime - bpmTimeLast) > 50) //don't update on very small differences
     //{
       //2. calculate the bpm
       tapTempoSw.tapBpm = (2 / tapTempoSw.tapTempoTime) * 60000; // (2(taps) / tapTempoTime(ms)) * 1min/60000ms
       tapTempoSw.tempoMultiplier = tapTempoSw.tapTempoTime / bpmTimeLast;
     
       //3. calculate the measure time
       tapTempoSw.tapTempoMeasureTime = (tapTempoSw.tapTempoSignature / tapTempoSw.tapTempoMeter) * tapTempoSw.tapTempoTime; 
     //}
     
     //start back at 0 on a substantial tempo change
     //if(abs(tapTempoSw.tapTempoTime - bpmTimeLast) > 500)
     //{
       //tremTimeBaseEncoder.selectedPulseNumber = 0;
       //tremTimeBaseEncoder.selectedPulsePart = 0;
     //}
     
     //apply the new scaling factor
     tapTimeScaling();
   }
   
   
}

/*******************************************************************
* tapTimeScaling() - use bpm and measure to update pulse times
********************************************************************/
void tapTimeScaling()
{
  //scale the pulse times to the tap tempo factor
  for(int j = 0; j < tremTimeBaseEncoder.pulseOnTimeVec.size(); j++)
  {
    tremTimeBaseEncoder.pulseOnTimeVec[j] = tremTimeBaseEncoder.pulseOnTimeVec[j] * tapTempoSw.tempoMultiplier;
    tremTimeBaseEncoder.pulseOffTimeVec[j] = tremTimeBaseEncoder.pulseOnTimeVec[j] * tapTempoSw.tempoMultiplier;
    
//       Serial.print("ON=");
//       Serial.print(tremTimeBaseEncoder.pulseOnTimeVec[j]);
//       Serial.print(", ");
//       Serial.print("OFF=");
//       Serial.print(tremTimeBaseEncoder.pulseOffTimeVec[j]);
//       Serial.print(", ");
//       Serial.print("selected=");
//       Serial.print(j);
//       Serial.print(", ");
//       Serial.print("factor=");
//       Serial.print(tapTempoSw.tempoMultiplier);
//       Serial.print("\n");
    
  }
  
  //reset encoder reading to match updated pulse value
  if(tremTimeBaseEncoder.selectedPulsePart == 0)
  {EncoderBus.resetEncValue(tremTimeBaseEncoder.pinNumber, tremTimeBaseEncoder.pulseOnTimeVec[tremTimeBaseEncoder.selectedPulseNumber]);}
  else 
  {EncoderBus.resetEncValue(tremTimeBaseEncoder.pinNumber, tremTimeBaseEncoder.pulseOffTimeVec[tremTimeBaseEncoder.selectedPulseNumber]);}
}


void tapTempoBLink()
{
  if(millis() - tapTempoStartTime >= 10)
  {
    digitalWrite(TAP_TEMPO_LED_OUTPUT, LOW);
  }
  if(millis() - tapTempoStartTime >= tapTempoSw.tapTempoTime)
  {
    tapTempoStartTime = millis();
    digitalWrite(TAP_TEMPO_LED_OUTPUT, HIGH);
  }
}


/**********************************************************************************************************
saveConfig() - save config to the EEPROM
**********************************************************************************************************/
void saveConfig()
{
  int eepromAddr = 0;
  //write ON times
  for(int i = 0; i < tremTimeBaseEncoder.pulseOnTimeVec.size(); i++)
  {
    for(int j = 0; j < 2 ; j++) 
    {
      EEPROM.write(eepromAddr, tremTimeBaseEncoder.pulseOnTimeVec[i] & (0xFF >> i * 2));
      eepromAddr++;
    }
  }
  
  //write OFF times
  for(int i = 0; i < tremTimeBaseEncoder.pulseOffTimeVec.size(); i++)
  {
    for(int j = 0; j < 2 ; j++) 
    {
      EEPROM.write(eepromAddr, tremTimeBaseEncoder.pulseOffTimeVec[i] & (0xFF >> i * 2));
      eepromAddr++;
    }
  }
  
  //write enabled state
  for(int i = 0; i < tremTimeBaseEncoder.pulseEnabledVec.size(); i++)
  {
    for(int j = 0; j < 2 ; j++) 
    {
      EEPROM.write(eepromAddr, tremTimeBaseEncoder.pulseEnabledVec[i] & (0xFF >> i * 2));
      eepromAddr++;
    }
  }
}



/**********************************************************************************************************
loadConfig() - load config from the EEPROM
**********************************************************************************************************/
void loadConfig()
{
  int eepromAddr = 0;
  //write ON times
  for(int i = 0; i < tremTimeBaseEncoder.pulseOnTimeVec.size(); i++)
  {
    for(int j = 0; j < 2 ; j++) 
    {
      tremTimeBaseEncoder.pulseOnTimeVec[i] & (0xFF << i * 2) = EEPROM.read(eepromAddr);
      eepromAddr++;
    }
  }
  
  //write OFF times
  for(int i = 0; i < tremTimeBaseEncoder.pulseOffTimeVec.size(); i++)
  {
    for(int j = 0; j < 2 ; j++) 
    {
      tremTimeBaseEncoder.pulseOffTimeVec[i] & (0xFF << i * 2) = EEPROM.read(eepromAddr);
      eepromAddr++;
    }
  }
  
  //write enabled state
  for(int i = 0; i < tremTimeBaseEncoder.pulseEnabledVec.size(); i++)
  {
    for(int j = 0; j < 2 ; j++) 
    {
      tremTimeBaseEncoder.pulseEnabledVec[i] & (0xFF << i * 2) = EEPROM.read(eepromAddr);
      eepromAddr++;
    }
  }
}

//wrapper functions
void _A_FALL(){EncoderBus.A_FALL();}
void _A_RISE(){EncoderBus.A_RISE();}
void _B_FALL(){EncoderBus.B_FALL();}
void _B_RISE(){EncoderBus.B_RISE();}


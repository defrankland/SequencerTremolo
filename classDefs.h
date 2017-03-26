#ifndef _USE_CLASS_DEFS_h
#define _USE_CLASS_DEFS_h

#include "rotaryEncoder.h"
#include "vector.h"

class TremPulseDev : public EncoderDevice
{
	public:
           Vector<int> pulseOnTimeVec, pulseOffTimeVec, pulseEnabledVec;
           int numberOfPulses;
           int selectedPulseNumber;
           int selectedPulsePart;
    
};

class TremSwDev: public EncoderDevice
{
	public:
           TremSwDev():lastPressTime(0), longPressTimeout(0), longPressDoneFlag(false),
                        longPressTime(2000){}; //constructor            
           int swValue; //on or off 
           long lastPressTime; //length of time that the button has been held down
           int longPressTime;  //length of time to press button to initiate long press
           int longPressTimeout;
           int longPressCheckState;
           boolean longPressDoneFlag;   
};

class TapTempoSw: public TremSwDev
{
        public:
          TapTempoSw():tempoMultiplier(0), tapTempoSignature(4), tapTempoMeter(4), applyTapTempo(false),
                       tapTempoTimeout(2000){};
          int tapTempoSignature; //4/4 = 4, 3/4 = 3 ...
          int tapTempoMeter; //4/4 = 4, 4/8 = 8 ...
          int tapTempoMeasureTime; //length of a measure (ms)
          float tapTempoTime; //when switch is pressed twice, time between presses. recalculate with existing if within timeout.  
          long tapTempoSwTimePress;
          int tapTempoTimeout;
          float tempoMultiplier;
          boolean applyTapTempo;
          int tapBpm; 
};


#endif //_USE_CLASS_DEFS_h

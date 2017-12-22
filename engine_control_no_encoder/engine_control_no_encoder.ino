/* This is the control program for a steam engine that uses a 5 port 3 position closed center solenoid valve to drive an industrial air cylinder.
 *  My previous engine designs used a rotary encoder, but this one will use a magnet attached to the piston rod that travels past a set of 4 hall sensors. 
 *  Two at TDC and BDC, and two that are in the middle that can be physically moved to control cutoff. This should make things waaaay simpler.
 */

//using the button parts of the automation library to simplify the cylinder position sensing
#include <Automaton.h>    


#include "Arduino.h"

#include <digitalWriteFast.h>  // library for high performance reads and writes by jrraines
                               // see http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1267553811/0
                               // and http://code.google.com/p/digitalwritefast/
 


//disable debugging once everything works
//#define EnableDebug

#ifdef EnableDebug
unsigned long previousMillis = 0;  
const long interval = 1000;
#endif


static int TDCPin = 2;
static int BDCPin = 3;
static int TopCutoffPin = 4;
static int BottomCutoffPin = 5;
static int EngineEnablePin = 7;
static int Solenoid1Pin = 10;     //solenoid 1 controls top solenoid
static int Solenoid2Pin = 11;     //solenoid 2 controls bottom solenoid
static int AdvancePin = 6;




int CycleState = 1;
/*  
 *  1 = top cylinder admission
 *  2 = top cylinder expansion
 *  3 = bottom cylinder admission
 *  4 = bottom cylinder expansion
 */

int Advance = 0;
int EEnable = 0;



Atm_button TDC, BDC, TopCutoff, BottomCutoff;

void setup()
{
  #ifdef EnableDebug
  Serial.begin(115200);
  #endif
 
 
  
  pinMode(EngineEnablePin, INPUT_PULLUP);
  pinMode(AdvancePin, INPUT_PULLUP);

  TDC.begin(TDCPin)
    .debounce(0)
    .onPress(TDC_Press)
    .onRelease(TDC_Release);

  BDC.begin(BDCPin)
    .debounce(0)
    .onPress(BDC_Press)
    .onRelease(BDC_Release);

  TopCutoff.begin(TopCutoffPin)
    .debounce(0)
    .onPress(TopCutoff_Press)
    .onRelease(TopCutoff_Release);

  BottomCutoff.begin(BottomCutoffPin)
    .debounce(0)
    .onPress(BottomCutoff_Press)
    .onRelease(BottomCutoff_Release);

  pinMode(Solenoid1Pin, OUTPUT);    
  pinMode(Solenoid2Pin, OUTPUT);    

  

}
 
void loop()
{
  //this loop includes no delays or anything, should execute as fast as reasonable


   //read switch states
   Advance = digitalReadFast(AdvancePin);
   EEnable = digitalReadFast(EngineEnablePin);


   //state program
   automaton.run();
   
   
   //all the valve control is handled in this sub
   SolenoidControl();


   
   #ifdef EnableDebug

   unsigned long currentMillis = millis();

   if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    Serial.print("Advance Pin:");
    Serial.println(Advance);
    Serial.print("State:");
    Serial.println(CycleState);
   }
   #endif
   
   
   
}

//when advance is enabled, the cylinder states change on rising. When advance is disabled, cylinder states change on falling.
void TDC_Press(int idx, int v, int up) {
  if (Advance == 0) {
    CycleState = 1;
  }
}

void TopCutoff_Press(int idx, int v, int up) {
  if (Advance == 0) {
    if (CycleState == 1) {
      CycleState = 2;
    }
  }
}

void BDC_Press(int idx, int v, int up) {
  if (Advance == 0) {
    CycleState = 3;
  }
}

void BottomCutoff_Press(int idx, int v, int up) {
  if (Advance == 0) {
    if (CycleState == 3) {
      CycleState = 4;
    }
  }
}

//advance disabled
void TDC_Release(int idx, int v, int up) {
  if (Advance == 1) {
    CycleState = 1;
  }
}

void TopCutoff_Release(int idx, int v, int up) {
  if (Advance == 1) {
    if (CycleState == 1) {
      CycleState = 2;
    }
  }
}

void BDC_Release(int idx, int v, int up) {
  if (Advance == 1) {
    CycleState = 3;
  }
}

void BottomCutoff_Release(int idx, int v, int up) {
  if (Advance == 1) {
    if (CycleState == 3) {
      CycleState = 4;
    }
  }
}



//here is the important part.
 
void SolenoidControl() {

  //disable the engine if the engine enable switch is off
  if (EEnable == 1) {
    digitalWriteFast(Solenoid1Pin, LOW);
    digitalWriteFast(Solenoid2Pin, LOW);
    return;
  }


  //top admission
  if (CycleState == 1) {
    digitalWriteFast(Solenoid1Pin, HIGH);
    digitalWriteFast(Solenoid2Pin, LOW);
  }
  //top expansion
  if (CycleState == 2) {
    digitalWriteFast(Solenoid1Pin, LOW);
    digitalWriteFast(Solenoid2Pin, LOW);
  }
  //bottom admission
  if (CycleState == 3) {
    digitalWriteFast(Solenoid1Pin, LOW);
    digitalWriteFast(Solenoid2Pin, HIGH);
  }
  //bottom expansion
  if (CycleState == 4) {
    digitalWriteFast(Solenoid1Pin, LOW);
    digitalWriteFast(Solenoid2Pin, LOW);
  }  
  
}
  
  
  
  
  
  
  

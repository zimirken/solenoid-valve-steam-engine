#include "Arduino.h"

#include <digitalWriteFast.h>  // library for high performance reads and writes by jrraines
                               // see http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1267553811/0
                               // and http://code.google.com/p/digitalwritefast/
 
// It turns out that the regular digitalRead() calls are too slow and bring the arduino down when
// I use them in the interrupt routines while the motor runs at full speed creating more than
// 40000 encoder ticks per second per motor.
 
// Quadrature encoder

//#define EncoderIsReversed

//disable debugging once everything works
#define EnableDebug


volatile bool _EncoderBSet;
volatile long EngineAngle = 0;


static int EncoderInterruptPin = 2;
static int EncoderPin2 = 3;
static int EngineEnablePin = 7;
static int AngleResetPin = 8;
static int Solenoid1Pin = 11;
static int Solenoid2Pin = 12;
static int Solenoid3Pin = 4;
static int Solenoid4Pin = 5;
static int ThrottlePin = A0;


static int TicksPerRev = 360;

//set the starting positions for each cylinder solenoid
static int Sol1Start = 0;
static int Sol2Start = 180;
static int Sol3Start = 90;
static int Sol4Start = 270;
static int SolStop = 180;    //when the solenoid should turn off, offset from the starting position

static int ResetAngle = 0;
static long SpeedInterval = 50; //measurement interval used to calculate the speed

//speed maps
static int Speed1 = 20; //start of speed 1 map
static int Speed2 = 40; //start of speed 2 map
static int Speed3 = 80; //start of speed 3 map

//throttle maps
static int ThrottleStart = 300; //start of throttle map
static int ThrottleEnd = 400; //end of throttle map

//cutoff minimum and maximum in degrees
static int CutoffMin = 40;
static int CutoffMax = 150;


//advance map
static int Advance[] = {0,10,20,30};




//variables for speed calculation
long previousMillis = 0;        // will store last time LED was updated 
int EngineSpeed = 0;
int LastAngle = 0;
int ThrottleSetting = 0;
int SpeedSet = 0;
int Sol1Off = 0;
int Sol2Off = 0;
int Sol3Off = 0;
int Sol4Off = 0;

 
void setup()
{
  #ifdef EnableDebug
  Serial.begin(115200);
  #endif
 
 
  // Quadrature encoder
  pinMode(EncoderInterruptPin, INPUT);      // sets pin A as input
  digitalWrite(EncoderInterruptPin, HIGH);  // turn on pullup resistors
  pinMode(EncoderPin2, INPUT);      // sets pin B as input
  digitalWrite(EncoderPin2, HIGH);  // turn on pullup resistors
  pinMode(EngineEnablePin, INPUT);
  digitalWrite(EngineEnablePin, HIGH);
  pinMode(AngleResetPin, INPUT);
  digitalWrite(AngleResetPin, HIGH);

  pinMode(Solenoid1Pin, OUTPUT);
  pinMode(Solenoid2Pin, OUTPUT);
  pinMode(Solenoid3Pin, OUTPUT);
  pinMode(Solenoid4Pin, OUTPUT);
  
     //angle reset for encoder calibration
   while (digitalReadFast(AngleResetPin) == 0) {  //if controller is powered on with the reset sensor on the magnet, it won't give a good read
     delay(200);                                  //this while loop waits for the sensor to turn off before starting the calibration loop
   }
   
   while (digitalReadFast(AngleResetPin) == 1){   //wait until the angle reset sensor turns on

   }
   
     EngineAngle = ResetAngle;
     LastAngle = ResetAngle;
     
  //interrupts are only enabled after calibration
  attachInterrupt(EncoderInterruptPin, EncoderInterruptA, RISING);
 
}
 
void loop()
{
  //this loop includes no delays or anything, should execute as fast as reasonable
  
   unsigned long currentMillis = millis();
   
   
   //speed measurement and calculation
   if(currentMillis - previousMillis > SpeedInterval) {
    previousMillis = currentMillis;  
    int LastEngineSpeed = EngineAngle - LastAngle ;
    if (LastEngineSpeed > 0) {
      EngineSpeed = LastEngineSpeed;
    }
    LastAngle = EngineAngle;
   }
   
   ThrottleSetting = analogRead(ThrottlePin);
   if (ThrottleSetting < ThrottleStart) {
    ThrottleSetting = ThrottleStart;
   }
   if (ThrottleSetting > ThrottleEnd) {
    ThrottleSetting = ThrottleEnd;
   }
   
   }
   
   
   
   
   //all the valve control is handled in this sub
   SolenoidControl(EngineAngle,EngineSpeed,ThrottleSetting,digitalReadFast(EngineEnablePin));


   
   #ifdef EnableDebug
   Serial.print("Engine Angle:");
   Serial.println(EngineAngle);
   Serial.print("Throttle setting:");
   Serial.println(ThrottleSetting);
   Serial.print("Engine Speed:");
   Serial.println(EngineSpeed);
   #endif
   
   
   

   
   
}
 
// Interrupt service routines for the quadrature encoder
void EncoderInterruptA()
{
  // Test transition; since the interrupt will only fire on 'rising' we don't need to read pin A
  _EncoderBSet = digitalReadFast(EncoderPin2);   // read the input pin
 
  // and adjust counter + if A leads B
  #ifdef EncoderIsReversed
    EngineAngle -= _EncoderBSet ? -1 : +1;
  #else
    EngineAngle += _EncoderBSet ? -1 : +1;
  #endif
  
  if (EngineAngle >= TicksPerRev) EngineAngle = 0;
  if (EngineAngle < 0) EngineAngle = (TicksPerRev - 1);
    
}


//here is the important part.
 
void SolenoidControl(int EAngle, int ESpeed, int TSetting, int EEnable) {

  //disable the engine if the engine enable switch is off
  if (EEnable == 1) {
    digitalWriteFast(Solenoid1Pin, LOW);
    digitalWriteFast(Solenoid2Pin, LOW);
    digitalWriteFast(Solenoid3Pin, LOW);
    digitalWriteFast(Solenoid4Pin, LOW);
    return;
  }

  
  //this advances the appearent angle of the crank based on the speed settings
  //in effect, advancing the timing
  if (ESpeed < Speed1) {
    SpeedSet = 0;
  }
  if (ESpeed >= Speed1 && ESpeed < Speed2) {
    SpeedSet = 1;
  }
  if (ESpeed >= Speed2 && ESpeed < Speed3) {
    SpeedSet = 2;
  }
  if (ESpeed >= Speed3) {
    SpeedSet = 3;
  }


  //if the advanced angle becomes invalid, make it valid
  EAngle = EAngle + Advance[SpeedSet];          
  if (EAngle >= TicksPerRev) {                  
    EAngle = EAngle - TicksPerRev;
  }
  
  

  //this figures out what angle the solenoids are supposed to turn off
  Sol1Off = Sol1Start + map(TSetting, ThrottleStart, ThrottleEnd, CutoffMin, CutoffMax);
  Sol2Off = Sol2Start + map(TSetting, ThrottleStart, ThrottleEnd, CutoffMin, CutoffMax);
  Sol3Off = Sol3Start + map(TSetting, ThrottleStart, ThrottleEnd, CutoffMin, CutoffMax);
  Sol4Off = Sol4Start + map(TSetting, ThrottleStart, ThrottleEnd, CutoffMin, CutoffMax);
  
  if (Sol1Off >= TicksPerRev) {
    Sol1Off = Sol1Off - TicksPerRev;
  }
  if (Sol2Off >= TicksPerRev) {
    Sol2Off = Sol2Off - TicksPerRev;
  }
  if (Sol3Off >= TicksPerRev) {
    Sol3Off = Sol3Off - TicksPerRev;
  }
  if (Sol4Off >= TicksPerRev) {
    Sol4Off = Sol4Off - TicksPerRev;
  }
  
  
  
  //This part acutally sets the valves
  if ((Sol1Off>Sol1Start && (EAngle > Sol1Start && EAngle < Sol1Off)) || (Sol1Off<Sol1Start && (EAngle > Sol1Start || EAngle < Sol1Off))) {
    digitalWriteFast(Solenoid1Pin, HIGH);
  }
  else {
    digitalWriteFast(Solenoid1Pin, LOW);
  }
  
  if ((Sol2Off>Sol2Start && (EAngle > Sol2Start && EAngle < Sol2Off)) || (Sol2Off<Sol2Start && (EAngle > Sol2Start || EAngle < Sol2Off))) {
    digitalWriteFast(Solenoid2Pin, HIGH);
  }
  else {
    digitalWriteFast(Solenoid2Pin, LOW);
  }
  
  if ((Sol3Off>Sol3Start && (EAngle > Sol3Start && EAngle < Sol3Off)) || (Sol3Off<Sol3Start && (EAngle > Sol3Start || EAngle < Sol3Off))) {
    digitalWriteFast(Solenoid3Pin, HIGH);
  }
  else {
    digitalWriteFast(Solenoid3Pin, LOW);
  }

  if ((Sol4Off>Sol4Start && (EAngle > Sol4Start && EAngle < Sol4Off)) || (Sol4Off<Sol4Start && (EAngle > Sol4Start || EAngle < Sol4Off))) {
    digitalWriteFast(Solenoid4Pin, HIGH);
  }
  else {
    digitalWriteFast(Solenoid4Pin, LOW);
  }
  



  
  
  
  
  
  
  
  
}
  
  
  
  
  
  
  

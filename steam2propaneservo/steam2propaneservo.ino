#include <Servo.h>
#include <LowPower.h>

/*
steam scooter control program




*/



//Port assignments
//analog inputs
//const int batteryvoltagepin = A1;
const int pressuresensorpin = A0;
const int waterlevelanalog = A3;

//digital inputs

const int throttle = 11;
const int power = 9;



//digital outputs
const int valveservo = 3;
const int waterpump = 7;
const int pressuredisplaypin = 6;         //neopixel string for pressure display and battery display
//const int batterydisplay = 9;          //single neopixel for battery display
const int waterpumpdisplay = 13;         //led lights up when water is detected

//timers and other constants
const int waterpumpinterval = 15;      //interval to check the water level at
const int shutdowntimer = 20;         //timer to slowly shutdown the pellet burner to avoid excessive smoke or overheating
const int glowplugtimer = 90;         //glowplug firing duration on startup or relight
const int pressurehigh = 100;          //maximum pressure, fan to idle, pressure may go higher due to idling fan and relief valve
const int pressurelow = 70;            //low pressure setting for fan control
const int PIXEL_COUNT = 8;              //neopixel count
const int batterypixel = 7;            //battery pixel
const float batteryhigh = 13.0;        //high voltage battery level
const float batterylow = 11.0;         //low voltage battery level
const float batterydivider = 0.25;      //ratio for battery voltage divider.
const long startuptime = 10;         //timer for propane ignition.
const int waterlevelset = 400;        //set point for analog water level sensor
const int pressureoff = 120;          //pressure for turning off propane burner.

const int propaneoff = 10;            //servo position for propane off
const int propaneprelight = 30;      //servo position for propane prelight, gas on but before ignition.
const int propanehigh = 50;           //servo position for high burn.
const int propanelow = 100;         //servo position for low burn.


long waterprevious = 0;
long shutdownprevious = 0;
int powerbuttonState = 0;         // current state of the button
int powerlastButtonState = 0;     // previous state of the button
int pressure = 0;
int temperature1 = 0;            
float temperature2 = 0;
long batterylevel = 0;
unsigned long currentMillis = 0;
int startupcheck = 0;
long startuptimer = 0;
int waterlevela = 0;

Servo propane;





void setup() {
  //congifure ports
  pinMode(throttle, INPUT);
  digitalWrite(throttle, HIGH);
  pinMode(power, INPUT);
  digitalWrite(power, HIGH);
  pinMode(waterpump, OUTPUT);
  pinMode(waterpumpdisplay, OUTPUT);
  Serial.begin(9600);
  digitalWrite(waterpump,LOW);

  propane.attach(valveservo);
  

}

void loop() {
  
  currentMillis = millis(); //update current time
//  temperature1 = 100*analogRead(thermocouple1pin); //update temperature
  if (analogRead(waterlevelanalog) > waterlevelset ) {
    waterlevela = 1;
  }
  if (analogRead(waterlevelanalog) < waterlevelset) {
    waterlevela = 0;
  }
  digitalWrite(waterpumpdisplay,waterlevela);
  //battery voltage
  //batterylevel = map((analogRead(batteryvoltagepin)/batterydivider),0,1024,0,5);
  
  //read pressure sensor and convert
  pressure = map(analogRead(pressuresensorpin),102,922,0,200); //maps the analog read to the pressure, this is based on the specs of the pressure sensor  
  
  Serial.print("Pressure:");
  Serial.println(pressure);
  Serial.print("Temperature1:");
  Serial.println(temperature1);
  Serial.print("Water Level:");
  Serial.println(analogRead(waterlevelanalog));
  Serial.print("Power Switch:");
  Serial.println(digitalRead(power));
  
  
  //shutdown section
  powerbuttonState = digitalRead(power);
    // compare the buttonState to its previous state
    if (powerbuttonState != powerlastButtonState) {
      shutdownprevious = currentMillis;
      startuptimer = currentMillis;
      powerlastButtonState = powerbuttonState;
    }      
  if (0==digitalRead(power)) {    
    shutdownsequence();
    startupcheck = 1;
  }
   
  if (currentMillis - startuptimer > (startuptime*1000)) {
    startupcheck = 0;
  }
  //check and maintain water level every interval
  if(currentMillis - waterprevious > (waterpumpinterval*1000)) {
    waterprevious = currentMillis;
    waterpumpfunc();
  }
   
  //this function controls the neopixel strip
//  pressuredisplay();
  
  //pressure based fire control function
  if (1==digitalRead(power)) {  
    pressurecontrol();
  }
  
  delay(200);  
  
  
}






void shutdownsequence() {
  if(currentMillis - shutdownprevious > (shutdowntimer*1000)) {
    propane.write(propaneoff);
   // digitalWrite(waterpump, 0);
    
    
    while(0 == digitalRead(power)){
      LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
    }
  }
  else{
   propane.write(propaneoff);
    
  }
}
  
  
//function turns on water pump if water sensor is low until water sensor is high         I should add code later to prevent boiler flooding    
void waterpumpfunc(){
 if (analogRead(waterlevelanalog) > waterlevelset ) {
   digitalWrite(waterpump, HIGH);
   //digitalWrite(waterpumpdisplay, HIGH);
   while (analogRead(waterlevelanalog) > waterlevelset ) {
       Serial.print("Pressure:");
  Serial.println(pressure);
  Serial.print("Temperature1:");
  Serial.println(temperature1);
  Serial.print("Water Level:");
  Serial.println(analogRead(waterlevelanalog) );
  Serial.print("Power Switch:");
  Serial.println(digitalRead(power));
     delay(10);
   }
   digitalWrite(waterpump, LOW);
   //digitalWrite(waterpumpdisplay, LOW);
 }
}
  
  



//the function that controls the fan and glow plug based on pressure and temperature
void pressurecontrol() {      
  //ignition state
  if (startupcheck == 1 && pressure < pressurelow) {
    propane.write(propaneprelight);
    return;
  }
  if (pressure > pressureoff) {
    propane.write(propaneoff);
    startupcheck = 1;
    return;
  }
  //low pressure state
  if (pressure < pressurelow) {
    propane.write(propanehigh);
    return;
  }
  //high pressure state
  if (pressure > pressurehigh) {
    propane.write(propanelow);
    return;
  }
  //medium pressure state, the important one
  if (pressure < pressurehigh && pressure > pressurelow) {
    propane.write(map(pressure, pressurelow, pressurehigh, propanehigh, propanelow));
    return;
  }
}






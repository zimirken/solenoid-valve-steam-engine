//#include <MAX6675.h>

//#include <Adafruit_MAX31855.h>

#include <LowPower.h>

//#include <Adafruit_NeoPixel.h>
//#include <SPI.h>
/*
steam scooter control program




*/



//Port assignments
//analog inputs
//const int batteryvoltagepin = A1;
const int pressuresensorpin = A0;
const int thermocouple1pin = A1;
const int waterlevelanalog = A3;

//digital inputs
const int waterlevel = 2;
const int throttle = 11;
const int power = 9;

// Example creating a thermocouple instance with software SPI on any three
// digital IO pins.
//#define DO   12
//#define CS   11
//#define CLK  10
////Adafruit_MAX31855 thermocouple(CLK, CS, DO);
// int CS = 11;             // CS pin on MAX6675
// int SO = 12;              // SO pin of MAX6675
// int SCK = 10;             // SCK pin of MAX6675
// int units = 2;            // Units to readout temp (0 = raw, 1 = ˚C, 2 = ˚F)
//MAX6675 temp(CS,SO,SCK,units);

//digital outputs
const int blowerfan = 3;
const int waterpump = 7;
const int glowplug = 8;
const int pressuredisplaypin = 6;         //neopixel string for pressure display and battery display
//const int batterydisplay = 9;          //single neopixel for battery display
const int waterpumpdisplay = 13;         //led lights up when water is detected

//timers and other constants
const int waterpumpinterval = 15;      //interval to check the water level at
const int shutdowntimer = 60;         //timer to slowly shutdown the pellet burner to avoid excessive smoke or overheating
const int glowplugtimer = 90;         //glowplug firing duration on startup or relight
const int idleblowerspeed = 100;        //speed to run blower fan at when idling
const int maxblowerspeed = 200;        //maximum blower speed if blower is oversized
const int pressurehigh = 100;          //maximum pressure, fan to idle, pressure may go higher due to idling fan and relief valve
const int pressurelow = 50;            //low pressure setting for fan control
const int glowplugshutofftemp = 50;  //thermocouple temperature to shutoff glowplug
const int glowplugontemp = 50;        //thermocouple temperature to turn on glowplug in case of a fire loss
const int PIXEL_COUNT = 8;              //neopixel count
const int batterypixel = 7;            //battery pixel
const float batteryhigh = 13.0;        //high voltage battery level
const float batterylow = 11.0;         //low voltage battery level
const float batterydivider = 0.25;      //ratio for battery voltage divider.
const long startuptime = 60;       //timer for glowplug
const int waterlevelset = 400;        //set point for analog water level sensor


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


//neopixel configuration
//Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, pressuredisplaypin, NEO_GRB + NEO_KHZ800);







void setup() {
  //congifure ports
  pinMode(waterlevel, INPUT);
  pinMode(throttle, INPUT);
  digitalWrite(throttle, HIGH);
  pinMode(power, INPUT);
  digitalWrite(power, HIGH);
  pinMode(blowerfan, OUTPUT);
  pinMode(waterpump, OUTPUT);
  pinMode(glowplug, OUTPUT);
  pinMode(waterpumpdisplay, OUTPUT);
  analogWrite(blowerfan,0);
  Serial.begin(9600);
  digitalWrite(glowplug,LOW);
  digitalWrite(waterpump,LOW);
  
//  strip.begin();
//  strip.show(); // Initialize all pixels to 'off'
}

void loop() {
  
  currentMillis = millis(); //update current time
  temperature1 = 100*analogRead(thermocouple1pin); //update temperature
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
    digitalWrite(blowerfan,0);
   // digitalWrite(waterpump, 0);
    digitalWrite(glowplug,LOW);
    
//    for(uint16_t i=0; i<strip.numPixels(); i++) {
//      strip.setPixelColor(i, 0, 0, 0);
//    }
//    strip.show();
    
    while(0 == digitalRead(power)){
      LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
    }
  }
  else{
    analogWrite(blowerfan, idleblowerspeed);
    digitalWrite(glowplug,LOW);
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
  
  
//void pressuredisplay() {
//  if (pressure < 20) {
//    strip.setPixelColor(0,0,map(pressure,0,20,0,255),0);
//    strip.setPixelColor(1,0,0,0);
//    strip.setPixelColor(2,0,0,0);
//    strip.setPixelColor(3,0,0,0);
//    strip.setPixelColor(4,0,0,0);
//    strip.setPixelColor(5,0,0,0);
//    strip.setPixelColor(6,0,0,0);
//  }
//  if (pressure > 20 && pressure < 40) {
//    strip.setPixelColor(0,0,255,0);
//    strip.setPixelColor(1,0,map(pressure,20,40,0,255),0);
//    strip.setPixelColor(2,0,0,0);
//    strip.setPixelColor(3,0,0,0);
//    strip.setPixelColor(4,0,0,0);
//    strip.setPixelColor(5,0,0,0);
//    strip.setPixelColor(6,0,0,0);
//  }
//  if (pressure > 40 && pressure < 60) {
//    strip.setPixelColor(0,0,255,0);
//    strip.setPixelColor(1,0,255,0);
//    strip.setPixelColor(2,0,map(pressure,20,40,0,255),0);
//    strip.setPixelColor(3,0,0,0);
//    strip.setPixelColor(4,0,0,0);
//    strip.setPixelColor(5,0,0,0);
//    strip.setPixelColor(6,0,0,0);
//  }
//  if (pressure > 60 && pressure < 80) {
//    strip.setPixelColor(0,0,255,0);
//    strip.setPixelColor(1,0,255,0);
//    strip.setPixelColor(2,0,255,0);
//    strip.setPixelColor(3,0,map(pressure,20,40,0,255),0);
//    strip.setPixelColor(4,0,0,0);
//    strip.setPixelColor(5,0,0,0);
//    strip.setPixelColor(6,0,0,0);
//  }
//  if (pressure > 80 && pressure < 100) {
//    strip.setPixelColor(0,0,255,0);
//    strip.setPixelColor(1,0,255,0);
//    strip.setPixelColor(2,0,255,0);
//    strip.setPixelColor(3,0,255,0);
//    strip.setPixelColor(4,0,map(pressure,20,40,0,255),0);
//    strip.setPixelColor(5,0,0,0);
//    strip.setPixelColor(6,0,0,0);
//  }
//  if (pressure > 100 && pressure < 120) {
//    strip.setPixelColor(0,0,255,0);
//    strip.setPixelColor(1,0,255,0);
//    strip.setPixelColor(2,0,255,0);
//    strip.setPixelColor(3,0,255,0);
//    strip.setPixelColor(4,0,255,0);
//    strip.setPixelColor(5,0,map(pressure,20,40,0,255),0);
//    strip.setPixelColor(6,0,0,0);
//  }
//  if (pressure > 120) {
//    strip.setPixelColor(0,0,255,0);
//    strip.setPixelColor(1,0,255,0);
//    strip.setPixelColor(2,0,255,0);
//    strip.setPixelColor(3,0,255,0);
//    strip.setPixelColor(4,0,255,0);
//    strip.setPixelColor(5,0,255,0);
//    strip.setPixelColor(6,0,255,0);
//  }
//  
//  
//  
//  if (batterylevel < batterylow) {
//    strip.setPixelColor(7,0,255,0);
//  }
//  if (batterylevel > batterylow && batterylevel < batteryhigh) {
//    strip.setPixelColor(7,0,0,255);
//  }
//  if (batterylevel > batteryhigh) {
//    strip.setPixelColor(7,255,0,0);
//  }
//  
//  //update the pixels
//  strip.show();
//}


//the function that controls the fan and glow plug based on pressure and temperature
void pressurecontrol() {      
  //ignition state
  if (startupcheck == 1) {
    digitalWrite(glowplug,HIGH);
    analogWrite(blowerfan,idleblowerspeed);
    return;
  }
  //low pressure state
  if (/*temperature1 > glowplugshutofftemp && */pressure < pressurelow) {
    digitalWrite(glowplug, LOW);
    analogWrite(blowerfan, maxblowerspeed);
    return;
  }
  //high pressure state
  if (/*temperature1 > glowplugshutofftemp && */pressure > pressurehigh) {
    digitalWrite(glowplug, LOW);
    analogWrite(blowerfan, idleblowerspeed);
    return;
  }
  //medium pressure state, the important one
  if (/*temperature1 > glowplugshutofftemp && */pressure < pressurehigh && pressure > pressurelow) {
    digitalWrite(glowplug, LOW);
    analogWrite(blowerfan, map(pressure, pressurelow, pressurehigh, maxblowerspeed, idleblowerspeed));
    return;
  }
}






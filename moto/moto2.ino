#include <Adafruit_MCP4725.h> //dac
#include <Adafruit_MLX90614.h> //i2c multiplex
#include <Wire.h>  //12c bus scanning
#include <SoftwareSerial.h> // talk to pi

//display
//refresh
unsigned long startMillis;
unsigned long currentMillis;
const unsigned long period = 1000 * 5;  //the value is a number of milliseconds
//pi display
SoftwareSerial piSerial(5, 6); // RX, TX    (tx is flagged)
//dac display
Adafruit_MCP4725 dac;
// volt display switch
const int PIN_BUTTON = 7;
volatile int oldButtonState = 0;
volatile int currDispl = 0;
const int DISP_TEMP1 = 0;
const int DISP_TEMP2 = 1;
//const int DISP_HZ = 2;


//temperature
Adafruit_MLX90614 irTemp1 = Adafruit_MLX90614();
Adafruit_MLX90614 irTemp2 = Adafruit_MLX90614();
//multiplexing
#define TCAADDR 0x70
extern "C" { 
#include "utility/twi.h"  // from Wire library, so we can do bus scanning
}
const int TCA_ADDR_1 = 2;
const int TCA_ADDR_2 = 7;

//hall effect
int PIN_HALL = 2;
volatile unsigned int tickCount = 0;
const int TACH_SAMPLE_SIZE = 5;
volatile unsigned long lastTachSample;


void tcaselect(uint8_t i) {
  if (i > 7) return;
 
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();  
}

void setup() {
  
  //displays setup
  Serial.begin(9600);
  piSerial.begin(9600);
  startMillis = millis();  //initial start time
  dac.begin(0x62); //0x62 is default address
  //display switch
  pinMode(PIN_BUTTON, INPUT);

  //temp setup
  tcaselect(TCA_ADDR_1);
  irTemp1.begin();
  tcaselect(TCA_ADDR_2);
  irTemp2.begin();

  //hall settup
  pinMode(PIN_HALL, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_HALL), hall_ISR, FALLING);
  
  piSerial.println("Hello?");
}

void loop() {
  // volt display switch
  int buttonState = digitalRead(PIN_BUTTON);
  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH && oldButtonState == LOW) {
    Serial.print(currDispl);
    Serial.print(" => ");
    currDispl = (currDispl + 1) % 2;
    Serial.println(currDispl);
  }

  oldButtonState = buttonState;

//  temperature
  tcaselect(TCA_ADDR_1);
  double temp1 = irTemp1.readObjectTempF();
  tcaselect(TCA_ADDR_2);
  double temp2 = irTemp2.readObjectTempF();

  //tach
  if (tickCount >= TACH_SAMPLE_SIZE){
    detachInterrupt(digitalPinToInterrupt(PIN_HALL));
    unsigned long timeBetweenFalls = millis() - lastTachSample;
    double xcpms =  (double) tickCount / (double) timeBetweenFalls; 
    double hz = xcpms * 1000;
    Serial.println(hz);
    attachInterrupt(digitalPinToInterrupt(PIN_HALL), hall_ISR, FALLING);
    lastTachSample = millis();
  }

  //display
  currentMillis = millis();  //get the current "time" (actually the number of milliseconds since the program started)
  if (currentMillis - startMillis >= period)  //test whether the period has elapsed
  {
    //serial
    Serial.print(temp1);Serial.print(",");Serial.print(temp2);
    Serial.println();
    
    //pi
    piSerial.print(temp1);piSerial.print(",");piSerial.print(temp2);
    piSerial.println();
    startMillis = currentMillis;  //IMPORTANT to save the start time of the current LED state.
    
    //dac
    double v = 0;
    if (currDispl == DISP_TEMP1){
      v = rescaleTempToV(temp1);
    } else if (currDispl == DISP_TEMP2){
      v = rescaleTempToV(temp2);
    }
    dac.setVoltage(v, false);
  }
}

double rescaleTempToV(double value){
  double newMin = 0;
  double newMax = 4095;
  double oldMin = 0;
  double oldMax = 484;

  double newV = (newMax - newMin) / (oldMax - oldMin) * (value - oldMin) + newMin;
  return newV;
}

void hall_ISR() {
  ++tickCount;
  Serial.println("tick");
}

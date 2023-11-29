#include <Wire.h>
#include <Zumo32U4.h>

Zumo32U4LineSensors lineSensors;
Zumo32U4Motors motors;
Zumo32U4ButtonA buttonA;
Zumo32U4ButtonB buttonB;
Zumo32U4OLED display;

int16_t lasterror = 0;



Zumo32U4Encoders encoders;
int16_t previousCountLeft = 0;
int16_t previousCountRight = 0;




const uint16_t maxSpeed = 200;
bool firstfault = true;

const unsigned long interval = 2000;
unsigned long previousMillis = 0;
unsigned long currentMillis;

unsigned int lineSensorValues[5];

void setup(){
    lineSensors.initFiveSensors();
    buttonA.waitForButton();
    calibrateLineSensors();
    Serial.begin(9600);
}

void speedometer(){
    static uint8_t lastDisplayTime;
  static uint8_t displayErrorLeftCountdown = 0;
  static uint8_t displayErrorRightCountdown = 0;
  if ((uint8_t)(millis() - lastDisplayTime) >= 100)
  {
     lastDisplayTime = millis();

    int16_t countsLeft = encoders.getCountsLeft();
    int16_t countsRight = encoders.getCountsRight();

    int16_t newCountLeft = countsLeft - previousCountLeft;
    int16_t newCountRight = countsRight - previousCountRight;
    int16_t avrage = (newCountLeft+newCountRight)/2;
    float distanse = 75.81*12;
    float meters = avrage/distanse*1.225221135;
    
    display.clear();
    display.print(meters);
    previousCountLeft = countsLeft;
    previousCountRight = countsRight;


}
}

void calibrateLineSensors(){
     delay(1000);
  for(uint16_t i = 0; i < 100; i++)
  {
    motors.setSpeeds(-200, 200);
    lineSensors.calibrate();
    }
   
  motors.setSpeeds(0, 0);
  delay(2000);
}


void follow(){


}

void loop(){
    speedometer();
    int16_t position = lineSensors.readLine(lineSensorValues);
    /*display.gotoXY(0,0);
    display.print(position);*/

    int16_t error = position - 2000;
    
    int16_t speedifference = error/4 + 6*(error-lasterror);

    lasterror = error;

    int leftSpeed = (int16_t)maxSpeed + speedifference;
    int rightSpeed = (int16_t)maxSpeed - speedifference;

 

    leftSpeed = constrain(leftSpeed,0,(int16_t)maxSpeed);
    rightSpeed = constrain(rightSpeed,0,(int16_t)maxSpeed);
    motors.setSpeeds(leftSpeed,rightSpeed);

}
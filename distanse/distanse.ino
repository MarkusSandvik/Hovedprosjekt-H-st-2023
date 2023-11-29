#include <Wire.h>
#include <Zumo32U4.h>

Zumo32U4Encoders encoders;
Zumo32U4Motors motors;
Zumo32U4ButtonA buttonA;
Zumo32U4ButtonC buttonC;
Zumo32U4OLED display;
int16_t distanse = 0;
int16_t lastAvrage = 0;

void setup()
{
Serial.begin(9600);
}


void regneDistanse(){
    long countsLeft = encoders.getCountsLeft();
    long countsRight = encoders.getCountsRight(); 

    long average = (countsLeft + countsRight)/2;

    float round = 75.81*12;
    distanse =(abs(average)/round)*12.5221135;
    lastAvrage = average;

    
     static uint8_t lastDisplayTime;
  if ((uint8_t)(millis() - lastDisplayTime) >= 100){
    lastDisplayTime = millis();
    display.clear();
    display.gotoXY(0,1);
    display.print(distanse);
    }
}

void loop()
{
    regneDistanse();

if (buttonA.isPressed())
  {
    motors.setSpeeds(400, 400);
  }
  else if (buttonC.isPressed())
  {
    motors.setSpeeds(-400, -400);
  }
  else
  {
    motors.setSpeeds(0, 0);
  }


}
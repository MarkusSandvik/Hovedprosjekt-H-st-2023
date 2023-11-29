#include <Wire.h>
#include <Zumo32U4.h>

Zumo32U4Encoders encoders;
Zumo32U4Buzzer buzzer;
Zumo32U4Motors motors;
Zumo32U4ButtonA buttonA;
Zumo32U4ButtonC buttonC;
int16_t previousCountLeft = 0;
int16_t previousCountRight = 0;


Zumo32U4OLED display;

void setup()
{
Serial.begin(9600);
}

char report[80];
void loop()
{
  static uint8_t lastDisplayTime;
  static uint8_t OneSecond = 1000;

    long countsLeft = encoders.getCountsLeft();
    long countsRight = encoders.getCountsRight();
    if ((uint8_t)(millis() - lastDisplayTime) >= 100)
  {
    int16_t newCountLeft = countsLeft - previousCountLeft;
    int16_t newCountRight = countsRight - previousCountRight;
    int16_t avrage = (newCountLeft+newCountRight)/2;
    float distanse = 75.81*12;
    float oneRound = 12.25221135;
    float meters = avrage/distanse*oneRound;
    Serial.println(meters);
    previousCountLeft = countsLeft;
    previousCountRight = countsRight;
    display.clear();
    display.print(meters);
    lastDisplayTime = millis();

}

if (buttonA.isPressed())
  {
    motors.setSpeeds(200, 200);
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
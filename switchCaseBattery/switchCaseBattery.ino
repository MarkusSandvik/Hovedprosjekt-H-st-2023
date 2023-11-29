#include <Wire.h>
#include <Zumo32U4.h>

Zumo32U4OLED display;
Zumo32U4ButtonA buttonA;
Zumo32U4ButtonB buttonB;
Zumo32U4Buzzer buzzer;

// Variables for showBatteryStatus()
unsigned long previousMillis = 0;
unsigned long refreshPreviousMillis = 0;
long displayTime = 0;
bool batteryDisplayed = false;

// Global Variables
int8_t batteryLevel = 100;
int8_t iAmSpeed = 20;
int8_t distance = 100;
int8_t timesCharged = 0;
int8_t batteryHealth = 8;


void setup(){

}

void loop(){
    showBatteryStatus();
        if(buttonA.isPressed()){
        batteryLevel = 7;
    }

    if(buttonB.isPressed()){
        batteryLevel = 2;
    }
}


void showBatteryStatus(){  
    long onInterval;
    long offInterval;
    long refreshInterval;
    uint8_t batteryCase; 
  if((batteryLevel < 10) && (batteryLevel > 5)){
        batteryCase = 0;
    }else if((batteryLevel < 5) && (batteryLevel > 0)){
        batteryCase = 1;
    }else if(batteryLevel == 0){
        batteryCase = 2;
    }


    switch (batteryCase)
    {
    case 0:
        onInterval = 5000;
        offInterval = 2000;
        refreshInterval = 500;
        break;
    case 1:
        onInterval = 2000;
        offInterval = 1000;
        refreshInterval = 500;
        break;
    case 2:
        //batteryCase2(); skal være når batteriet er helt utladet. Her må vi kunne legge inn en hidden feature som
        //gjør at vi kan få litt strøm slik at den kommer seg til ladestasjonen.
        break;
    
    default:
        onInterval = 10000;
        offInterval = 2000;
        refreshInterval = 500;
        break;
    }



    unsigned long currentMillis = millis();



    if (batteryDisplayed == false){
        if (currentMillis - refreshPreviousMillis >= refreshInterval){
            //float speedReading = speedometer();
            display.clear();
            display.setLayout11x4();                        // Divide screen into 11 columns and 4 rows
            display.print(F("Speed:"));
            display.gotoXY(0,1);
            display.print(iAmSpeed);
            display.gotoXY(7,1);
            display.print(F("m/s"));
            display.gotoXY(0,2);
            display.print(F("Distance:"));
            display.gotoXY(0,3);
            display.print(distance);
            display.gotoXY(7,3);
            display.print(F("cm"));
            refreshPreviousMillis = currentMillis;
        } // end if
    } // end if


    if (currentMillis - previousMillis >= onInterval){
        display.clear();
        display.setLayout21x8();                        // Divide screen into 21 columns and 8 rows
        display.print(F("Battery level"));
        display.gotoXY(15,0);
        display.print(batteryLevel);
        display.gotoXY(0,2);
        display.print(F("Times Charged"));
        display.gotoXY(15,2);
        display.print(timesCharged);
        display.gotoXY(0,4);
        display.print(F("Battery Health"));
        display.gotoXY(15,4);
        display.print(batteryHealth);
        if(batteryLevel < 10){
            display.gotoXY(0,6);
            display.print("Please recharge!");
        }
        previousMillis = currentMillis;
        displayTime = currentMillis;
        batteryDisplayed = true;                        // To make the next if sentence only run once after this text have been ran
    } // end if

    if ((currentMillis - displayTime >= offInterval) && (batteryDisplayed == true)){
        batteryDisplayed = false;
    } // end if

}//end void showBatteryStatus






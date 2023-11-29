#include <Wire.h>
#include <Zumo32U4.h>

/////////// NOTES ////////////
/*
- Add switchcase for display modes/ buzzer                              | DONE
- Add switchcase in softwareBattery for special functions               |
- Add lineFollower                                                      | DONE
- Add switchcase in line follower for turning, job etc.                 |
- Add Random based taxi job                                             | DONE (might need adjustment in payment calculation and random(LOW,HIGH))
- Fix speed and distance calculation. Use encoders.getCountsAndReset    |
- Add function to activate / deactivate hiddenfeature                   | DONE
- Add function to activate emergency charging                           | Skal vi bare aktivere den med tastetrykk på fjernkontrollen?
*/

Zumo32U4OLED display;
Zumo32U4Encoders encoders;
Zumo32U4Buzzer buzzer;
Zumo32U4Motors motors;
Zumo32U4ButtonA buttonA;
Zumo32U4ButtonB buttonB;
Zumo32U4ButtonC buttonC;
Zumo32U4LineSensors lineSensors;
Zumo32U4IMU imu;

// Variables for softwareBattery
int8_t batteryLevel = 90;
long lastDistance = 0;
float consumptionMeasure = 0;
int8_t timesCharged = 0;
unsigned long batteryMillis = 0;

// Variables for hiddenFeature()
bool hiddenActivated = false;
bool emergencyChargingUsed = false;
bool emergencyChargeMode = false;
bool countDownStarted = false;
bool firstStage = false;
unsigned long waitForStageTwo = 0;
unsigned long countDownStart = 0;
const long countDownInterval = 15000;

// Variables for showBatteryStatus()
unsigned long previousMillis = 0;
unsigned long refreshPreviousMillis = 0;
long displayTime = 0;
bool batteryDisplayed = false;

// Variables for speedometer
int16_t previousCountLeft = 0;
int16_t previousCountRight = 0;

// Variables for regneDistanse()
int16_t lastAverage = 0;
long distance = 0;
unsigned long distanceMillis = 0;

// Variables for taxiDriver()
int workCase = 0;
bool passengerFound = false;
bool onDuty = false;
unsigned long searchTime = 0;
unsigned long missionStart = 0;
unsigned long missionDistance = 0;
unsigned long startDistance = 0;
unsigned long passengerEnteredMillis = 0;
unsigned long previousWorkRequest = 0;
const long freeTimeInterval = 15000;

// Variables for followLine
unsigned int lineSensorValues[5];
int16_t lastError = 0;
const uint16_t maxSpeed = 200;

// Variables for charging
int missingAmount = 0;
int account = 100;
int debit = 0;


///////// TEST VARIABLES ////
int batteryHealth = 2;
float iAmSpeed = 0;



void setup(){
    Serial.begin(9600);
    Wire.begin();
    imu.init();
    imu.enableDefault();
    lineSensors.initFiveSensors();

    // Wait for button A to be pressed and released.
    display.clear();
    display.print(F("Press A"));
    display.gotoXY(0, 1);
    display.print(F("to start"));
    buttonA.waitForButton();
    //calibrateLineSensors();
    
} // end setup

void loop(){
    //hiddenFeature();
    chargingMode();
} // end loop

void hiddenFeature(){
    int currentMillis = millis();
    imu.read();
    Serial.println(imu.g.x);
    Serial.println(firstStage);

    int8_t averageSpeed = 0; //speedometer();
    unsigned long distanceChange = distance - lastDistance;

    // Function to turn on hiddenActivated
    if ((imu.g.x > 15000) and (firstStage == false)){
        firstStage = true;
        waitForStageTwo = currentMillis;
    } // end if

    if (firstStage == true){
        if (imu.g.y > 15000){
            hiddenActivated = true;
            ledRed(1);
            display.clear();
            display.print(F("Hidden feature"));
        } // end if
    } // end if

    if ((currentMillis - waitForStageTwo > 5000) and (firstStage == true)){
        firstStage = false;
    } // end if

    // Function to turn off hiddenActivated
    if ((hiddenActivated == true) && (countDownStarted == false)){
        countDownStart = currentMillis;
        countDownStarted = true;
        display.clear();
        display.setLayout21x8();
        display.print(F("HiddenFeature deactivated"));
    } // end if

    if (currentMillis - countDownStart > countDownInterval){
        hiddenActivated = false;
        firstStage = false;
        ledRed(0);
    } // end if

    // Function to turn on emergencyChargingMode
    


    if (hiddenActivated == true){ // FINN PÅ NOE SOM SKAL AKTIVERE FUNSKJONEN
      
        lastDistance = distance;

        consumptionMeasure += (averageSpeed / distanceChange); // EKSEMPEL PÅ FUNKSJON, OPPDATER NÅR VI TESTER MED DATA

        if (consumptionMeasure <= -10){
            if ((emergencyChargeMode == true) && (emergencyChargingUsed = false)){
                batteryLevel += 20;
                emergencyChargingUsed = true;
            } // end if

            else{
                batteryLevel += 2;
            } // end else
        } // end else
    } // end if
} // end void

void chargingMode(){
    unsigned long currentMillis = millis();
    batteryLevel = constrain(batteryLevel, 0, 100);
    if ((debit > 0) && (account >= debit)){
        account = account - debit;
        debit = 0;
        display.clear();
        display.print("Debit account cleared");
    } // end if

    if (batteryLevel == 100){
        display.clear();
        display.setLayout21x8();
        display.print(F("Battery full"));
        display.gotoXY(0,3);
        display.print(F("Battery = "));
        display.gotoXY(15, 3);
        display.print(batteryLevel);
        display.gotoXY(0,4);
        display.print(F("Bank account = "));
        display.gotoXY(15, 4);
        display.print(account);
        display.gotoXY(0, 5);
        display.print(F("Debit account = "));
        display.gotoXY(16, 5);
        display.print(debit);
        display.gotoXY(0,7);
        display.print(F("Press A to exit"));
        buttonA.waitForButton();
    } // end if

    if ((currentMillis % 500) == 0){                           // Just a idea, test for reliability
        display.clear();
        display.setLayout21x8();
        display.print(F("Charging mode"));
        display.gotoXY(0,2);
        display.print(F("A = add 10%"));
        display.gotoXY(0,3);
        display.print(F("B = add 50%"));
        display.gotoXY(0,4);
        display.print(F("C = Fully Charged"));
        display.gotoXY(0,5);
        display.print(F("Battery level ="));
        display.gotoXY(16, 5);
        display.print(batteryLevel);
        display.gotoXY(0,6);
        display.print(F("Bank account  = "));
        display.gotoXY(16, 6);
        display.print(account);
        display.gotoXY(0, 7);
        display.print(F("Debit account = "));
        display.gotoXY(16, 7);
        display.print(debit);
    } // end if

    if (buttonA.isPressed() == 1){
        if (account >= 10){
            account -= 10;
            batteryLevel += 10;
            display.clear();
            display.print(F("Wait while charging"));
            delay(1000);
        } // end if
        
        else{
            missingAmount = 10 - account;
            display.clear();
            display.setLayout21x8();
            display.print(F("You are missing"));
            display.gotoXY(16, 0);
            display.print(missingAmount);
            display.gotoXY(19, 0);
            display.print(F("kr"));
            display.gotoXY(0, 1);
            display.print(F("Charge on debit?"));
            display.gotoXY(0,2);
            display.print(F("A = Yes"));
            display.gotoXY(10,2);
            display.print(F("B = Cancel"));

            while ((buttonA.isPressed() == 0) and (buttonB.isPressed() == 0)){
            } // end while
            if (buttonA.isPressed() == 1){
                debit += missingAmount;
                account = 0;
                display.clear();
                display.print(F("Wait while charging"));
                delay(1000);
                chargingMode();
            } // end if

            if (buttonB.isPressed() == 1){
                // QUIT CHARGING MODE, Integrate with driving switch case?
            } // end if
        } // end else
    } // end if

    if (buttonB.isPressed() == 1){
        if (account >= 50){
            account -= 50;
            batteryLevel += 50;
            display.clear();
            display.print(F("Wait while charging"));
            delay(5000);
        } // end if
        
        else{
            missingAmount = 50 - account;
            display.clear();
            display.setLayout21x8();
            display.print(F("You are missing"));
            display.gotoXY(16, 0);
            display.print(missingAmount);
            display.gotoXY(19, 0);
            display.print(F("kr"));
            display.gotoXY(0, 1);
            display.print(F("Charge on debit?"));
            display.gotoXY(0,2);
            display.print(F("A = Yes"));
            display.gotoXY(10,2);
            display.print(F("B = Cancel"));

            while ((buttonA.isPressed() == 0) and (buttonB.isPressed() == 0)){
            } // end while
            if (buttonA.isPressed() == 1){
                debit += missingAmount;
                account = 0;
                display.clear();
                display.print(F("Wait while charging"));
                delay(5000);
                chargingMode();
            } // end if

            if (buttonB.isPressed() == 1){
                // QUIT CHARGING MODE, Integrate with driving switch case?
            } // end if
        } // end else
    } // end if

    if (buttonC.isPressed() == 1){
        int percentageUntilFull = (100 - batteryLevel);
        int chargingTime = percentageUntilFull * 100;
        if (account >= (percentageUntilFull)){
            account -= percentageUntilFull;
            batteryLevel += percentageUntilFull;
            display.clear();
            display.print(F("Wait while charging"));
            delay(chargingTime);
        } // end if
        
        else{
            missingAmount = percentageUntilFull - account;
            display.clear();
            display.setLayout21x8();
            display.print(F("You are missing"));
            display.gotoXY(16, 0);
            display.print(missingAmount);
            display.gotoXY(19, 0);
            display.print(F("kr"));
            display.gotoXY(0, 1);
            display.print(F("Charge on debit?"));
            display.gotoXY(0,2);
            display.print(F("A = Yes"));
            display.gotoXY(10,2);
            display.print(F("B = Cancel"));

            while ((buttonA.isPressed() == 0) and (buttonB.isPressed() == 0)){
            } // end while
            if (buttonA.isPressed() == 1){
                debit += missingAmount;
                account = 0;
                display.clear();
                display.print(F("Wait while charging"));
                delay(chargingTime);
                chargingMode();
            } // end if

            if (buttonB.isPressed() == 1){
                // QUIT CHARGING MODE, Integrate with driving switch case?
            } // end if
        } // end else
    } // end if
} // end void
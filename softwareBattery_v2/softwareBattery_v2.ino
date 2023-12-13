#include <Wire.h>
#include <Zumo32U4.h>
#include <EEPROM.h>

/////////// NOTES ////////////
/*
Dette er den orginale softwareBatterykoden før forbedring
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

// Variables for batteryConsumption()
int8_t batteryLevel = 100;
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
unsigned long previousLowBatteryMillis = 0;
long displayTime = 0;
bool batteryDisplayed = false;

// Variables for speedometerAndMeassureDistance()
long meassureDistance = 0;
float iAmSpeed = 0;

// Variables for charging()
bool chargingModeEntered = false;
int missingAmount = 0;
int account = 100;
int debit = 0;

// Variables for batteryLife()
int batteryHealth = 100;
int timesBelowFive = 0;
int lastMinuteAverageSpeed = 0;
int lastMinuteMaxSpeed = 0;
int averageSpeed = 0;
int maxSpeedLastMinute = 0;
bool incidentRegistered = false;
bool timerStarted = false;
bool motorRunning = false;
bool serviceDone = false;
bool productionFaultEffect = false;
unsigned long timeNearMaxSpeed = 0;
unsigned long aboveSeventyTimer = 0;
unsigned long runningHours = 0;
unsigned long runStartedAt = 0;
unsigned long minuteStartDistance = 0;
unsigned long randomProductionFault = 0;
unsigned long lastMinuteAboveSeventyPercent = 0;


///////// TEST VARIABLES ////



void setup(){
    Serial.begin(9600);
    Wire.begin();
    imu.init();
    imu.enableDefault();
    batteryHealth = EEPROM.read(0);

    randomProductionFault = random(pow(2,17), pow(2,19));

    // Wait for button A to be pressed and released.
    display.clear();
    display.print(F("Press A"));
    display.gotoXY(0, 1);
    display.print(F("to start"));
    buttonA.waitForButton();
    //EEPROM.write(0,100);
    display.setLayout21x8();
} // end setup

void loop(){
    SpeedometerAndMeassureDistance();
    softwareBattery();
    hiddenFeature();
    showBatteryStatus();
    chargingMode();
    batteryLife();
} // end loop

void batteryConsumption(){
    // This functions reduces the batteryLevel based on speed
    unsigned long currentMillis = millis();
    const int batteryLevelCalculationInterval = 200;

    if (currentMillis - batteryMillis > batteryLevelCalculationInterval){
    batteryMillis = currentMillis;
    consumptionMeasure += (abs(iAmSpeed)/30); // The distance measurement is based on the speed measurement, and we therefore did not use it in the calculation
    } // end if

    if (consumptionMeasure >= 10){
        batteryLevel -= 1;
        consumptionMeasure = 0;
    } // end if
    batteryLevel = constrain(batteryLevel, 0, 100);
} // end void

void speedometerAndMeassureDistance(){
    // This function calculates the current speed and increse the distance, and stores them in global variables
    static uint8_t lastDisplayTime;
    if ((uint8_t)(millis() - lastDisplayTime) >= 200)
    {
        long countsLeft = encoders.getCountsAndResetLeft();
        long countsRight = encoders.getCountsAndResetRight();
        float avrage = (countsLeft+countsRight)/2;
        float distance = 75.81*12;
        float oneRound = 12.25221135;
        float meters = avrage/distance*oneRound*5;
        iAmSpeed = meters;
        meassureDistance +=abs(meters)*0.2;
        lastDisplayTime = millis();
      } // end if
}// end void

void hiddenFeatureForCharging(){
    // Function to activate a "secret" mode where driving backwards will increase the battery level. 
    unsigned long currentMillis = millis();
    const int stageOneActiveInterval = 5000;
    const int countDownInterval = 15000;
    imu.read();

    // Functions to turn on hiddenActivated
    //To activate the first stage, tilt the car quicly to with the clock around the x-axis (looking at the car from behind)
    if ((imu.g.x > 15000) && (firstStage == false)){
        firstStage = true;
        waitForStageTwo = currentMillis;
    } // end if

    // To activate hiddenFeature firstStep must be active, and then the car must be tilted quickly forward
    if (firstStage == true){
        if (imu.g.y > 15000){
            hiddenFeatureActivated = true;
            ledGreen(1);
            display.clear();
            display.print(F("Hidden feature"));
        } // end if
    } // end if

    // Limits the time from first stage to second stage to maximum 5 seconds
    if ((currentMillis - waitForStageTwo > stageOneActiveInterval) && (firstStage == true)){
        firstStage = false;
    } // end if

    // Function to turn off hiddenFeatureActivated
    if ((hiddenFeatureActivated == true) && (countDownStarted == false)){
        countDownStart = currentMillis;
        countDownStarted = true;
    } // end if

    // Limits the active time for hiddenFeature to 15 seconds
    if ((currentMillis - countDownStart > countDownInterval) && (hiddenFeatureActivated == true)) {
        hiddenFeatureActivated = false;
        firstStage = false;
        ledGreen(0);
        display.clear();
        display.setLayout21x8();
        display.print(F("Hidden Feature deactivated"));
    } // end if

    if (hiddenFeatureActivated == true){ 

        if (currentMillis - batteryMillis > 100){
            batteryMillis = currentMillis;
            consumptionMeasure -= (abs(iAmSpeed)/30); // EKSEMPEL PÅ FUNKSJON, OPPDATER NÅR VI TESTER MED DATA
        } // end if

        if (consumptionMeasure <= -10){
            // Emergency charging regains ten times the amount of charging, but can only be used once
            if ((emergencyChargeMode == true) && (emergencyChargingUsed = false)){
                batteryLevel += 20;
                emergencyChargingUsed = true;
            } // end if

            else{
                batteryLevel += 2;
            } // end else
            consumptionMeasure = 0;
            batteryLevel = constrain(batteryLevel, 0, 100);
        } // end else
    } // end if
} // end void

void showBatteryStatus(){
    /*This will present different display layouts:
        Main layout: Speed and distance
        Battery layout: Battery level, Times charged and Battery Health
        Warning when battery is low
    */  
    long onInterval;
    long offInterval;
    long refreshInterval;
    uint8_t batteryCase;
    unsigned long currentMillis = millis();

    if(batteryLevel > 100){
        batteryCase = 0;
    } 
   else if((batteryLevel < 10) && (batteryLevel > 5)){
        batteryCase = 1;
    }else if((batteryLevel < 5) && (batteryLevel > 0)){
        batteryCase = 2;
    }else if(batteryLevel == 0){
        batteryCase = 3;
    }

    switch (batteryCase)
    {
    case 0:
        onInterval = 10000;
        offInterval = 2000;
        refreshInterval = 500;
        ledRed(0);
        break;
    case 1:
        onInterval = 5000;
        offInterval = 2000;
        refreshInterval = 500;
        if (currentMillis - previousLowBatteryMillis > 15000){
            display.print(F("Low Battery"));
            previousLowBatteryMillis = currentMillis;
            ledRed(1);
            buzzer.playFrequency(440,200,15);
            display.clear();
        } // end if
        break;
    case 2:
        onInterval = 2000;
        offInterval = 1000;
        refreshInterval = 500;
        if (currentMillis - previousLowBatteryMillis > 15000){
            display.print(F("Low Battery"));
            motors.setSpeeds(0,0);
            ledRed(1);
            buzzer.playFrequency(440,100,15);
            delay(150);                                                                     // kan den løses uten delay? Trenger vi å løse uten delay?
            buzzer.playFrequency(440,100,15);
            display.clear();
            previousLowBatteryMillis = currentMillis;
        } // end if
        break;
    case 3:
        driveModeController = code3;
        display.clear();
        display.setLayout21x8();
        display.gotoXY(2,1);
        display.print(F("Battery is empty"));
        display.gotoXY(6,4);
        display.print(F("GAME OVER"));
        while (batteryLevel = 0){
        } // end while
        break;
    
    default:
        onInterval = 10000;
        offInterval = 2000;
        refreshInterval = 500;
        break;
    } // end switch

    if (batteryDisplayed == false){
        if (currentMillis - refreshPreviousMillis >= refreshInterval){
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
            display.print(meassureDistance);
            display.gotoXY(7,3);
            display.print(F("cm"));
            refreshPreviousMillis = currentMillis;
        } // end if
    } // end if

    if (currentMillis - previousMillis >= onInterval){
        batteryHealth = EEPROM.read(0);                 // Reads the batteryHealth from the EEPROM
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
}//end void

void chargingMode(){
    unsigned long currentMillis = millis();

    motors.setSpeeds(0,0);

    if (chargingModeEntered == false){
        chargingModeEntered = true;
        timesCharged += 1;
    } // end if

    if ((debit > 0) && (account >= debit)){
        account = account - debit;
        debit = 0;
        display.clear();
        display.print("Debit account cleared");
        delay(500);
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
        driveModeController = code1;
        chargingModeEntered = false;
    } // end if

    else{             
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
        while((buttonA.isPressed() == 0) && (buttonB.isPressed() == 0) && (buttonC.isPressed() == 0)){
        }//end while
    } // end else

    if (buttonA.isPressed() == 1){
        if (account >= 10){
            account -= 10;
            batteryLevel += 10;
            display.clear();
            display.print(F("Wait while charging"));
            batteryLevel = constrain(batteryLevel, 0, 100);
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

            while ((buttonA.isPressed() == 0) && (buttonB.isPressed() == 0)){
            } // end while
            if (buttonA.isPressed() == 1){
                debit += missingAmount;
                account = 0;
                batteryLevel += 10;
                display.clear();
                display.print(F("Wait while charging"));
                batteryLevel = constrain(batteryLevel, 0, 100);
                delay(1000);
            } // end if

            if (buttonB.isPressed() == 1){
                chargingModeEntered = false;
                driveModeController = code1;
            } // end if
        } // end else
    } // end if

    if (buttonB.isPressed() == 1){
        if (account >= 50){
            account -= 50;
            batteryLevel += 50;
            display.clear();
            display.print(F("Wait while charging"));
            batteryLevel = constrain(batteryLevel, 0, 100);
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

            while ((buttonA.isPressed() == 0) && (buttonB.isPressed() == 0)){
            } // end while
            if (buttonA.isPressed() == 1){
                debit += missingAmount;
                account = 0;
                batteryLevel += 50;
                display.clear();
                display.print(F("Wait while charging"));
                batteryLevel = constrain(batteryLevel, 0, 100);
                delay(5000);
            } // end if

            if (buttonB.isPressed() == 1){
                chargingModeEntered = false;
                driveModeController = code1;
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
            batteryLevel = constrain(batteryLevel, 0, 100);
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

            while ((buttonA.isPressed() == 0) && (buttonB.isPressed() == 0)){
            } // end while
            if (buttonA.isPressed() == 1){
                debit += missingAmount;
                account = 0;
                batteryLevel += percentageUntilFull;
                display.clear();
                display.print(F("Wait while charging"));
                batteryLevel = constrain(batteryLevel, 0, 100);
                delay(chargingTime);
            } // end if

            if (buttonB.isPressed() == 1){
                chargingModeEntered = false;
                driveModeController = code1;
            } // end if
        } // end else
    } // end if
} // end void

void updateBatteryHealthEEPROM(){
    // Function to reduce times written to EEPROM
    if (batteryHealth != previousBatteryHealth){
            EEPROM.write(0,batteryHealth); // To reduce writing to EEPROM the value is only updated if value has changed
            previousBatteryHealth = batteryHealth;
        } // end if
} // end void

void batteryLife(){
    unsigned long currentMillis = millis();
    unsigned int oneMinute = 60000;

    if ((batteryHealth <= 5) && (incidentRegistered == false)){    // Record times batterylevel is less than or equal to 5%
        timesBelowFive += 1;
        incidentRegistered = true;
    } // end if

    else if (batteryHealth > 5){                                    // Code to prevent one incident recorded multiple times
        incidentRegistered = false;
    } // end else if

    if ((abs(iAmSpeed > 0)) && (motorRunning == false)){           // Record time when motors are running
        runStartedAt = currentMillis;
        motorRunning = true;
    } // end if

    else if (abs(iAmSpeed == 0) && (motorRunning == true)){        // Stop recording when motors are stopped
        runningHours += (currentMillis - runStartedAt);
        motorRunning = false;
    } // end else if


    if (iAmSpeed > maxSpeedLastMinute){                             // Record the maximum speed in this period
        maxSpeedLastMinute = iAmSpeed;
    } // end if

    if (iAmSpeed > (50 * 0,7) && (timerStarted == false)){        // Start recording time for motors going above 70% of absolute max speed
        aboveSeventyTimer = currentMillis;                          // when speed goes above 70%
        timerStarted = true;
    } // end if

    if (iAmSpeed < (50 * 0,7) && (timerStarted == true)){         // Stop recording time for motors going above 70% of absolute max speed
        timeNearMaxSpeed += (currentMillis - aboveSeventyTimer);    // when speed goes below 70% and add to total storage variable
        timerStarted = false;
    } // end if

    if ((runningHours >= oneMinute) || (((currentMillis - runStartedAt) + runningHours) >= oneMinute)){ // Runs once each minute
        timeNearMaxSpeed += (currentMillis - aboveSeventyTimer);                        // When motor has run for on minute
        lastMinuteAverageSpeed = (meassureDistance - minuteStartDistance) / oneMinute;  // Store average speed
        lastMinuteMaxSpeed = maxSpeedLastMinute;                                        // Store maxSpeed
        lastMinuteAboveSeventyPercent = timeNearMaxSpeed;                               // Store time above 70% of absolute maximum speed
        
        minuteStartDistance = meassureDistance;                                         // Reset variables for next minute running
        aboveSeventyTimer = 0;                                                      
        maxSpeedLastMinute = 0;
        runningHours = 0;
        motorRunning = false;
        timerStarted = false;

        // Calculation to reduce batteryHealth based on avearage speed, max speed, time above 70% of max speed, times battery have been below 5% and time battery have been charged
        batteryHealth -= round(((lastMinuteAverageSpeed / 10) + (constrain(lastMinuteMaxSpeed - 30, 0, 30) / 10) + (lastMinuteAboveSeventyPercent / 2000) + timesBelowFive + timesCharged));
        batteryHealth = constrain(batteryHealth, 0, 100);

        // Stores value for batteryHealth in EEPROM
        updateBatteryHealthEEPROM();
    } // end if

    // Function to reduce batteryHealth if batteryFault occurs. Reduses batteryLife by 50%
    if ((currentMillis >= randomProductionFault) && (productionFaultEffect == false)){
        batteryHealth -= 50;
        batteryHealth = constrain(batteryHealth, 0, 100);
        productionFaultEffect = true;
        motors.setSpeeds(0,0);
        display.clear();
        display.setLayout21x8();
        display.gotoXY(7,1);
        display.print(F("WARNING:"));
        display.gotoXY(4,3);
        display.print(F("Battery fault"));
        display.gotoXY(1,5);
        display.print(F("Battery health ="));
        display.gotoXY(19,5);
        display.print(batteryHealth);
        display.gotoXY(3,7);
        display.print(F("A = Acknowledge"));
        buttonA.waitForButton();
        updateBatteryHealthEEPROM();
    } // end if

    // Function to do a service if batteryHealth is below 50% and car have enough money
    if ((batteryHealth < 50) && (serviceDone == false)){ // Each battery can only have one Service
        if(account >= 100){
            motors.setSpeeds(0,0);
            display.clear();
            display.setLayout21x8();
            display.print(F("Battery need service"));
            display.gotoXY(0,2);
            display.print(F("Service price 100kr"));
            display.gotoXY(0,3);
            display.print(F("Bank account ="));
            display.gotoXY(16,3);
            display.print(account);
            display.gotoXY(0,5);
            display.print(F("Battery health ="));
            display.gotoXY(18,5);
            display.print(batteryHealth);
            display.gotoXY(0,7);
            display.print(F("A = Battery Service"));
            buttonA.waitForButton();
            batteryHealth += 10;
            account -= 100;
            serviceDone = true;
            updateBatteryHealthEEPROM();
        } // end if
    } // end if

    // Function to "change battery" when batteryHealth = 0;
    if (batteryHealth == 0){
        motors.setSpeeds(0,0);
        display.clear();
        display.setLayout21x8();
        display.print(F("Change battery"));
        display.gotoXY(0,2);
        display.print(F("Opperation price 150kr"));
        display.gotoXY(0,3);
        display.print(F("Bank account ="));
        display.gotoXY(16,3);
        display.print(account);
        display.gotoXY(0,5);
        display.print(F("Battery health ="));
        display.gotoXY(18,5);
        display.print(batteryHealth);
        display.gotoXY(0,7);
        display.print(F("A = Battery Service"));
        buttonA.waitForButton();
        account -= 150;
        batteryHealth = 100;
        serviceDone = false;
        timesCharged = 0;                           // Value reset when battery is changed
        timesBelowFive = 0;                         // Value reset when battery is changed
        updateBatteryHealthEEPROM();               
    } // end if
} // end void

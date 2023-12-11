#include <Wire.h>
#include <Zumo32U4.h>
#include <IRremote.h>
#include <EEPROM.h>

/////////// NOTES ////////////
/*
- Add switchcase for display modes/ buzzer                              | DONE
- Save batteryLevel in EEPROM                                           | DONE
- Create SensorNode (miniprosjekt?)                                     | DONE
- Add switchcase in softwareBattery for special functions               | 
- Add lineFollower                                                      | DONE
- Add switchcase in line follower for turning, job etc.                 | 
- Add Random based taxi job                                             | DONE (might need adjustment in payment calculation and random(LOW,HIGH))
- Fix speed and distance calculation. Use encoders.getCountsAndReset    | DONE
- Add function to activate / deactivate hiddenfeature                   | DONE
- Add function to activate emergency charging                           | Skal vi bare aktivere den med tastetrykk på fjernkontrollen?
- testestes
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
uint8_t batteryLevel = 100;
uint8_t timesCharged = 0;
float consumptionMeasure = 0;
unsigned long batteryMillis = 0;

//variables for IR remote
#define code1 3910598400 // 1 på IR fjernkontroll
#define code2 3860463360 // 2 på IR fjernkontroll
#define code3 4061003520 // 3 på IR fjernkontroll
#define yes 3175284480 // * på IR fjernkontroll
#define no 3041591040 // # på IR fjernkontrollen
#define upButton 3108437760 // pil opp på fjernkontrollen
#define downButton 3927310080 // pil ned på fjernkontrollen
#define zero 2907897600 //0 på IR fjernkontrollen

#define chargingStation 1253111734//dette er koden som gjør at bilen skal stoppe ved ladestasjonen
#define taxiPassenger  671389550//dette er hva ladestasjonen sender ut hvis det er passasjerer som venter.
const long RECV_PIN = A4;
IRrecv irrecv(RECV_PIN);
unsigned long irNum;
unsigned long driveModeController;
unsigned long taxiModeController;
unsigned long changeSpeedController;
/////////////////////////////


// Variables for hiddenFeature()
bool firstStage = false;
bool countDownStarted = false;
bool hiddenFeatureActivated = false;
bool emergencyChargingUsed = false;
bool emergencyChargeMode = false;
unsigned long waitForStageTwo = 0;
unsigned long countDownStart = 0;
const int countDownInterval = 15000;

// Variables for showBatteryStatus()
bool batteryDisplayed = false;
unsigned long previousMillis = 0;
unsigned long refreshPreviousMillis = 0;
unsigned long displayTime = 0;

// Variables for speedometerAndMeassureDistance()
unsigned long meassureDistance = 0;
float iAmSpeed = 0;

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
const int freeTimeInterval = 15000;

// Variables for followLine()
unsigned int lineSensorValues[5];
int16_t lastError = 0;
int maxSpeed = 200; //////////////////////// 200

// Variables for charging()
bool chargingStationDetected = false;
bool chargingModeEntered = false;
int missingAmount = 0;
int account = 100;
int debit = 0;

// Variables for batteryLife()
uint8_t batteryHealth = 100;
uint8_t previousBatteryHealth;
uint8_t timesBelowFive = 0;
uint8_t lastMinuteAverageSpeed = 0;
uint8_t lastMinuteMaxSpeed = 0;
uint8_t averageSpeed = 0;
uint8_t maxSpeedLastMinute = 0;
bool incidentRegistered = false;
bool timerStarted = false;
bool motorRunning = false;
bool serviceDone = false;
bool productionFaultEffect = false;
unsigned int timeNearMaxSpeed = 0;
unsigned int aboveSeventyTimer = 0;
unsigned int runningHours = 0;
unsigned int lastMinuteAboveSeventyPercent = 0;
unsigned long runStartedAt = 0;
unsigned long minuteStartDistance = 0;
unsigned long randomProductionFault = 0;


//Variables for buzzer
unsigned long buzzerMillis;
unsigned long buzzerPeriod;
///////// TEST VARIABLES ////



void setup(){
    Serial.begin(9600);
    Wire.begin();
    imu.init();
    imu.enableDefault();
    IrReceiver.begin(RECV_PIN, ENABLE_LED_FEEDBACK);
    batteryHealth = EEPROM.read(0);
    lineSensors.initFiveSensors();

    randomProductionFault = random(pow(2,17), pow(2,19));

    // Wait for button A to be pressed and released.
    display.clear();
    display.print(F("Press A"));
    display.gotoXY(0, 1);
    display.print(F("to start"));
    buttonA.waitForButton();
    //EEPROM.write(0,100);
    calibrateLineSensors();
} // end setup

void loop(){
    IrRemote();
    driveMode();
    SpeedometerAndMeassureDistance();
    batteryConsumption();
    //hiddenFeature();
    showBatteryStatus();
    //taxiDriver();
    //searchForPassenger();
    //drivePassenger();
    //followLine();
    //chargingMode();
    //batteryLife();
} // end loop

void IrRemote(){
	if(IrReceiver.decode()){
    
	irNum = IrReceiver.decodedIRData.decodedRawData;
    if((irNum == code1)||(irNum == code2) ||(irNum == code3)||(irNum == chargingStation)){
        driveModeController = irNum;
        }else if((irNum == yes)||(irNum == no)/*||(irNum == zero)*/){
            taxiModeController = irNum;
        }else if((irNum == upButton)||(irNum == downButton)){
            changeSpeedController = irNum;
        }else if(irNum == zero){
            passengerFound = true;
        }
    }
IrReceiver.resume();
}

void driveMode(){
    Serial.println(driveModeController);
    switch (driveModeController)
    {
    case code1:
        followLine();
        break;

    case code2:
        taxiDriver();
        followLine();
        break;

    case code3:
        motors.setSpeeds(0,0);
        break;

    case chargingStation:
        motors.setSpeeds(0,0);
        chargingMode();
        chargingStationDetected = true;
    default:
        break;
    }
}

void SpeedometerAndMeassureDistance(){
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
}// end voud SpeedometerAndMeassureDistance

void changeSpeed(){
    constrain(maxSpeed, 0, 400);
    if(changeSpeedController == upButton){
        maxSpeed += 20;
    }else if(changeSpeedController == downButton){
        maxSpeed -= 20;
    }
    changeSpeedController = 0;
}

void batteryConsumption(){
    unsigned long currentMillis = millis();

    if (currentMillis - batteryMillis > 100){
    batteryMillis = currentMillis;
    consumptionMeasure += (abs(iAmSpeed)/30); // EKSEMPEL PÅ FUNKSJON, OPPDATER NÅR VI TESTER MED DATA
    } // end if

    if (consumptionMeasure >= 10){
        batteryLevel -= 1;
        consumptionMeasure = 0;
    } // end if
    batteryLevel = constrain(batteryLevel, 0, 100);
} // end void

void hiddenFeature(){
    unsigned long currentMillis = millis();
    imu.read();
    Serial.println(imu.g.x);
    Serial.println(firstStage);

    // Function to turn on hiddenFeatureActivated
    if ((imu.g.x > 15000) and (firstStage == false)){
        firstStage = true;
        waitForStageTwo = currentMillis;
    } // end if

    if (firstStage == true){
        if (imu.g.y > 15000){
            hiddenFeatureActivated = true;
            ledRed(1);
            display.clear();
            display.print(F("Hidden feature"));
        } // end if
    } // end if

    if ((currentMillis - waitForStageTwo > 5000) and (firstStage == true)){
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
        ledRed(0);
        display.clear();
        display.setLayout21x8();
        display.print(F("HiddenFeature deactivated"));
    } // end if

    if (hiddenFeatureActivated == true){ 

        // Function to turn on emergencyChargingMode //////////////////////////////////////////////////////////// AMUND LEGG INN KNAPP FRA FJERNKONTROLL////////////////////////////
        /*
        if (AMUND LEGG INN KNAPPETRYKK){
            emergencyChargingMode = true;
        } // end if
        */
        if (currentMillis - batteryMillis > 100){
            batteryMillis = currentMillis;
            consumptionMeasure -= (abs(iAmSpeed)/30); // EKSEMPEL PÅ FUNKSJON, OPPDATER NÅR VI TESTER MED DATA
        } // end if
        consumptionMeasure -= (abs(iAmSpeed)/30); // EKSEMPEL PÅ FUNKSJON, OPPDATER NÅR VI TESTER MED DATA

        if (consumptionMeasure <= -10){
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
    long onInterval;
    long offInterval;
    long refreshInterval;
    uint8_t batteryCase;

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
        break;
    case 1:
        onInterval = 5000;
        offInterval = 2000;
        refreshInterval = 500;
        break;
    case 2:
        onInterval = 2000;
        offInterval = 1000;
        refreshInterval = 500;
        //batteryCase2(); skal være når batteriet er helt utladet. Her må vi kunne legge inn en hidden feature som
        //gjør at vi kan få litt strøm slik at den kommer seg til ladestasjonen.
        break;
    case 3:
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
        EEPROM.write(0,batteryHealth);
        batteryHealth = EEPROM.read(0);
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

void taxiDriver(){
    unsigned long currentMillis = millis();
    onDuty = true;
    Serial.print(workCase);
    switch (workCase)
    {
    case 1:
        searchForPassenger();
        break;
    case 2:
        drivePassenger();
        break;
    default:
        searchForPassenger();
        break;
    } // end case
    
} // end void

void searchForPassenger(){
    unsigned long currentMillis = millis();
    
    if (passengerFound == true){

            missionDistance = random(200,400);
            motors.setSpeeds(0,0);
            display.clear();
            display.setLayout21x8();
            display.print(F("The passenger want to travel"));
            display.gotoXY(0,1);
            display.print(F("travel for"));
            display.gotoXY(12,1);
            display.print(missionDistance);
            display.gotoXY(18,1);
            display.print(F("cm"));
            display.gotoXY(0,3);
            display.print(F("Do you take the job?"));
            display.gotoXY(0,4);
            display.print(F("A = YES"));
            display.gotoXY(15,4);
            display.print(F("B = NO"));
            display.gotoXY(0,5);
            display.print(F("C = Off duty"));
            passengerFound = false;
            while(taxiModeController == 0){
                IrRemote();
            }//end while
            if (taxiModeController == yes){
                delay(500);
                startDistance = meassureDistance;
                passengerEnteredMillis = currentMillis;
                workCase = 2;
            } // end if

            else if (taxiModeController == no){
                delay(500);
                workCase = 1;
                driveModeController = code2;
            } // end if

            else if (taxiModeController = zero){
                delay(500);
                driveModeController = code1;
            } // end if 
        taxiModeController = 0;   
        } // end if
} // end void

void drivePassenger(){
    unsigned long currentMillis = millis();
    if (meassureDistance - startDistance >= missionDistance){
        motors.setSpeeds(0,0);
        unsigned long payment = ((missionDistance * 2000) / (currentMillis - passengerEnteredMillis));
        account +=  payment;
        display.clear();
        display.setLayout21x8();
        display.print(F("Passenger delivered"));
        display.gotoXY(0,2);
        display.print(F("Payment:"));
        display.gotoXY(13,2);
        display.print(payment);
        display.gotoXY(18,2);
        display.print(F("kr"));
        display.gotoXY(0,4);
        display.print(F("Continue Working?"));
        display.gotoXY(0,5);
        display.print(F("A = Search for client"));
        display.gotoXY(0,6);
        display.print(F("B = End work"));
        while(taxiModeController == 0){
            IrRemote();
        }//end while
        if (taxiModeController == yes){
            delay(500);
            startDistance = meassureDistance;
            passengerEnteredMillis = currentMillis;
            workCase = 1;
        } // end if

        else if (taxiModeController == no){
            delay(500);
            driveModeController = code1;
        } // end if
        taxiModeController = 0;  
    } // end if
} // end void

void followLine(){
    int16_t position = lineSensors.readLine(lineSensorValues);
    /*display.gotoXY(0,0);
    display.print(position);*/

    int16_t error = position - 2000;
    
    int16_t speedifference = error/4 + 6*(error-lastError);

    lastError = error;

    int leftSpeed = (int16_t)maxSpeed + speedifference;
    int rightSpeed = (int16_t)maxSpeed - speedifference;

 

    leftSpeed = constrain(leftSpeed,0,(int16_t)maxSpeed);
    rightSpeed = constrain(rightSpeed,0,(int16_t)maxSpeed);
    motors.setSpeeds(leftSpeed,rightSpeed);
} // end void

void calibrateLineSensors(){
     delay(1000);
  for(uint16_t i = 0; i < 100; i++)
  {
    motors.setSpeeds(-200, 200);
    lineSensors.calibrate();
    } // end for
   
  motors.setSpeeds(0, 0);
  delay(2000);
} // end void

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

    else{             // Just a idea, test for reliability
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
        while((buttonA.isPressed() == 0) and (buttonB.isPressed() == 0) and (buttonC.isPressed() == 0)){
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

            while ((buttonA.isPressed() == 0) and (buttonB.isPressed() == 0)){
            } // end while
            if (buttonA.isPressed() == 1){
                debit += missingAmount;
                account = 0;
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

            while ((buttonA.isPressed() == 0) and (buttonB.isPressed() == 0)){
            } // end while
            if (buttonA.isPressed() == 1){
                debit += missingAmount;
                account = 0;
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

            while ((buttonA.isPressed() == 0) and (buttonB.isPressed() == 0)){
            } // end while
            if (buttonA.isPressed() == 1){
                debit += missingAmount;
                account = 0;
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

void batteryLife(){
    unsigned long currentMillis = millis();
    unsigned int oneMinute = 60000;

    if ((batteryHealth <= 5) and (incidentRegistered == false)){    // Record times batterylevel is less than or equal to 5%
        timesBelowFive += 1;
        incidentRegistered = true;
    } // end if

    else if (batteryHealth > 5){                                    // Code to prevent one incident recorded multiple times
        incidentRegistered = false;
    } // end else if

    if ((abs(iAmSpeed > 0)) and (motorRunning == false)){           // Record time when motors are running
        runStartedAt = currentMillis;
        motorRunning = true;
    } // end if

    else if (abs(iAmSpeed == 0) and (motorRunning == true)){        // Stop recording when motors are stopped
        runningHours += (currentMillis - runStartedAt);
        motorRunning = false;
    } // end else if


    if (iAmSpeed > maxSpeedLastMinute){                             // Record the maximum speed in this period
        maxSpeedLastMinute = iAmSpeed;
    } // end if

    if (iAmSpeed > (50 * 0,7) and (timerStarted == false)){        // Start recording time for motors going above 70% of absolute max speed
        aboveSeventyTimer = currentMillis;                          // when speed goes above 70%
        timerStarted = true;
    } // end if

    if (iAmSpeed < (50 * 0,7) and (timerStarted == true)){         // Stop recording time for motors going above 70% of absolute max speed
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

        batteryHealth -= round(((lastMinuteAverageSpeed / 10) + (constrain(lastMinuteMaxSpeed - 30, 0, 30) / 10) + (lastMinuteAboveSeventyPercent / 2000) + timesBelowFive + timesCharged));
        batteryHealth = constrain(batteryHealth, 0, 100);

        updateBatteryHealthEEPROM();
    } // end if

    if ((currentMillis >= randomProductionFault) and (productionFaultEffect == false)){
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

    if ((batteryHealth < 50) and (serviceDone == false)){ // Each battery can only have one Service
        if(account >= 100){
            motors.setSpeeds(0,0);
            display.clear();
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

    if (batteryHealth == 0){
        motors.setSpeeds(0,0);
        display.clear();
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

void updateBatteryHealthEEPROM(){
    // Function to reduce times written to EEPROM
    if (batteryHealth != previousBatteryHealth){
            EEPROM.write(0,batteryHealth); // To reduce writing to EEPROM the value is only updated if value has changed
            previousBatteryHealth = batteryHealth;
        } // end if
} // end void

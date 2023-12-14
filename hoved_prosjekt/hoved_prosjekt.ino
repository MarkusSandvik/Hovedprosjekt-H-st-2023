#include <Wire.h>
#include <Zumo32U4.h>
#include <IRremote.h>
#include <EEPROM.h>

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

#define ok 3208707840 // ok på fjernkontrollen

#define chargingStation 1253111734//dette er koden som gjør at bilen skal stoppe ved ladestasjonen
#define taxiPassenger 1652587174//dette er hva ladestasjonen sender ut hvis det er passasjerer som venter.
const long RECV_PIN = A4;
IRrecv irrecv(RECV_PIN);
unsigned long irNum;
unsigned long driveModeController;
unsigned long taxiModeController;
unsigned long changeSpeedController;

// Variables for hiddenFeatureForCharging()
bool firstStage = false;
bool countDownStarted = false;
bool hiddenFeatureActivated = false;
bool emergencyChargingUsed = false;
bool emergencyChargeMode = false;
unsigned long waitForStageTwo = 0;
unsigned long countDownStart = 0;

// Variables for showBatteryStatus()
bool batteryDisplayed = false;
unsigned long previousMillis = 0;
unsigned long refreshPreviousMillis = 0;
unsigned long previousLowBatteryMillis = 0;
unsigned long displayTime = 0;

// Variables for speedometerAndMeassureDistance()
unsigned long meassureDistance = 0;
float iAmSpeed = 0;

// Variables for taxiDriver()
int workCase = 0;
bool passengerFound = false;
bool onDuty = false;
unsigned int missionDistance = 0;
unsigned long startDistance = 0;
unsigned long passengerEnteredMillis = 0;

// Variables for followLine()
unsigned int lineSensorValues[5];
int16_t lastError = 0;
int maxSpeed = 300; 

// Variables for charging()
bool chargingStationDetected = false;
bool chargingModeEntered = false;
int chargingTime;
int missingAmount = 0;
int account = 10;
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

//Variables for wrongWayReverseAndTurn()
byte lastGivenCase;
bool trackIsLost = false;
bool reverseTimerStarted = false;
unsigned long reverseTimer = 0;

///////// TEST VARIABLES ////



void setup(){
    Serial.begin(9600);
    Wire.begin();
    imu.init();
    imu.enableDefault();
    IrReceiver.begin(RECV_PIN, ENABLE_LED_FEEDBACK);
    lineSensors.initFiveSensors();

    randomProductionFault = random(pow(2,17), pow(2,19));

    // Wait for button A to be pressed and released.
    display.clear();
    display.print(F("Press A"));
    display.gotoXY(0, 1);
    display.print(F("to start"));
    buttonA.waitForButton();
    //EEPROM.write(0,100);
    batteryHealth = EEPROM.read(0);
    calibrateLineSensors();
} // end setup

void loop(){
    IrRemote();
    driveMode();
    speedometerAndMeassureDistance();
    changeSpeed();
    batteryConsumption();
    hiddenFeatureForCharging();
    showBatteryStatus();
    batteryLife();
    wrongWayReverseAndTurn();
} // end loop

void IrRemote(){
	if(IrReceiver.decode()){
        irNum = IrReceiver.decodedIRData.decodedRawData;
        if((irNum == code1)||(irNum == code2)||(irNum == code3)||(irNum == chargingStation)){
            driveModeController = irNum;
        } // end if
        else if((irNum == yes)||(irNum == no)||(irNum == zero)){
            taxiModeController = irNum;
        } // end else if
        else if((irNum == upButton)||(irNum == downButton)){
            changeSpeedController = irNum;
        } // end else if
        else if(irNum == taxiPassenger){
            passengerFound = true;
        } // end else if
        else if (irNum == ok){
            emergencyChargeMode = true;
        } // end else if
    } // end if

    IrReceiver.resume();
} // end void

void driveMode(){
    // Switch case for different drivemodes
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
        if(batteryLevel <= 10){
            motors.setSpeeds(0,0);
            chargingMode();
        chargingStationDetected = true; 
        }
        else{
            driveModeController = code1;
        }
    break;
    default:
        break;
    } // end switch
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

void changeSpeed(){
    // Function to change the speed by using the remote
    constrain(maxSpeed, 0, 400);
    if(changeSpeedController == upButton){
        maxSpeed += 20;
    }else if(changeSpeedController == downButton){
        maxSpeed -= 20;
    }
    changeSpeedController = 0;
} // end void

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

void hiddenFeatureForCharging(){

    activationOfhiddenFeature();

    if (hiddenFeatureActivated == true){ 
        hiddenFeature();
    } // end if
    
    deactivationOfHiddenFeature();
} // end void

void activationOfhiddenFeature(){
    /* Function to activate a "secret" mode where driving backwards will increase the battery level. 
    -To activate the first stage, tilt the car quicly to with the clock around the x-axis (looking at the car from behind)
    -To activate hiddenFeature firstStep must be active, and then the car must be tilted quickly forward */
    unsigned long currentMillis = millis();
    const int stageOneActiveInterval = 5000;
    imu.read();

    if ((imu.g.x > 15000) && (firstStage == false)){
        firstStage = true;
        waitForStageTwo = currentMillis;
    } // end if

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
} // end void

void deactivationOfHiddenFeature(){
    unsigned long currentMillis = millis();
    const int countDownInterval = 15000;    // Limits the active time for hiddenFeature to 15 seconds
    
    if ((currentMillis - countDownStart > countDownInterval) && (hiddenFeatureActivated == true)) {
        hiddenFeatureActivated = false;
        firstStage = false;
        ledGreen(0);
        display.clear();
        display.setLayout21x8();
        display.print(F("Hidden Feature deactivated"));
    } // end if
} // end void

void hiddenFeature(){
    unsigned long currentMillis = millis();
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
} // end void

void showBatteryStatus(){
    /*This will present different display layouts:
    - Main layout: Speed and distance
    - Battery layout: Battery level, Times charged and Battery Health
    - Warning when battery is low / very low / empty
    */  
    uint8_t batteryCase;

    if(batteryLevel > 10){
        batteryCase = 0;
    } // end if
    else if((batteryLevel <= 10) && (batteryLevel > 5)){
        batteryCase = 1;
    } // end else if
    else if((batteryLevel < 5) && (batteryLevel > 0)){
        batteryCase = 2;
    }// end else if
    else if(batteryLevel == 0){
        batteryCase = 3;
    } // end else if

    if (batteryDisplayed == false){
        mainDisplayLayout();
    } // end if

    displayBatteryData();

    switch (batteryCase)
    {
    case 0:
        ledRed(0);
        break;
    case 1:
        lowBattery();
        break;
    case 2:
        veryLowBattery();
        break;
    case 3:
        driveModeController = code3;
        emptyBattery();
        break;
    
    default:
        ledRed(0);
    } // end switch  
}//end void

void mainDisplayLayout(){
    unsigned long currentMillis = millis();
    const int refreshInterval = 500;

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
} // end void

void displayBatteryData(){
    unsigned long currentMillis = millis();
    unsigned int onInterval = 10000;
    unsigned int offInterval = 2000;

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
        } // end if
        
        previousMillis = currentMillis;
        displayTime = currentMillis;
        batteryDisplayed = true;                        // To make the next if sentence only run once after this text have been ran
    } // end if

    if ((currentMillis - displayTime >= offInterval) && (batteryDisplayed == true)){
        batteryDisplayed = false;
    } // end if
} // end void

void lowBattery(){
    unsigned long currentMillis = millis();
    if (currentMillis - previousLowBatteryMillis > 15000){
        display.clear();
        display.print(F("Low Battery"));
        previousLowBatteryMillis = currentMillis;
        ledRed(1);
        buzzer.playFrequency(440,200,15);
    } // end if
} // end void

void veryLowBattery(){
    unsigned long currentMillis = millis();
    if (currentMillis - previousLowBatteryMillis > 15000){
        display.clear();
        display.print(F("Very low Battery"));
        motors.setSpeeds(0,0);
        ledRed(1);
        buzzer.playFrequency(440,100,15);
        delay(150);                                                                     // kan den løses uten delay? Trenger vi å løse uten delay?
        buzzer.playFrequency(440,100,15);
        previousLowBatteryMillis = currentMillis;
    } // end if
} // end void

void emptyBattery(){
    display.clear();
    display.setLayout21x8();
    display.gotoXY(2,1);
    display.print(F("Battery is empty"));
    display.gotoXY(6,4);
    display.print(F("GAME OVER"));
    while (batteryLevel == 0){
    } // end while
} // end void

void taxiDriver(){
    unsigned long currentMillis = millis();
    onDuty = true;
    switch (workCase)
    {
    case 1:
        searchForPassenger();
        break;
    case 2:
        maxSpeed = 320; 
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

            missionDistance = random(350,700);
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
        unsigned long payment = ((missionDistance * 3000) / ((currentMillis - passengerEnteredMillis)*0.05));
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

    chargerInitialization();

    if (batteryLevel == 100){
        batteryFull();
    } // end if

    else{             
        batteryChargerMenu();
    } // end else

    if (buttonA.isPressed() == 1){
        chargingTime = 1000;
        if (account >= 10){
            chargeTenPercent();
        } // end if
        
        else{
            missingAmount = 10 - account;
            chargeOnDebit(10);
        } // end else
    } // end if

    if (buttonB.isPressed() == 1){
        chargingTime = 5000;
        if (account >= 50){
            chargeFiftyPercent();
        } // end if
        
        else{
            missingAmount = 50 - account;
            chargeOnDebit(50);
        } // end else
    } // end if

    if (buttonC.isPressed() == 1){
        int percentageUntilFull = (100 - batteryLevel);
        chargingTime = percentageUntilFull * 100;
        if (account >= (percentageUntilFull)){
            chargeFullBattery(percentageUntilFull);
        } // end if
        
        else{
            missingAmount = percentageUntilFull - account;
            chargeOnDebit(percentageUntilFull);
        } // end else
    } // end if
} // end void

void chargerInitialization(){
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
} // end if

void batteryFull(){
    display.clear();
    display.setLayout21x8();
    display.print(F("Battery full"));
    display.gotoXY(0,3);
    display.print(F("Battery: "));
    display.gotoXY(17, 3);
    display.print(batteryLevel);
    display.gotoXY(0,4);
    display.print(F("Bank account: "));
    display.gotoXY(17, 4);
    display.print(account);
    display.gotoXY(0, 5);
    display.print(F("Debit account: "));
    display.gotoXY(17, 5);
    display.print(debit);
    display.gotoXY(0,7);
    display.print(F("Press A to exit"));
    buttonA.waitForButton();
    driveModeController = code1;
    chargingModeEntered = false;
} // end void

void batteryChargerMenu(){
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
} // end if

void chargeTenPercent(){
    account -= 10;
    batteryLevel += 10;
    display.clear();
    display.print(F("Wait while charging"));
    batteryLevel = constrain(batteryLevel, 0, 100);
    delay(chargingTime);
} // end if

void chargeFiftyPercent(){
    account -= 50;
    batteryLevel += 50;
    display.clear();
    display.print(F("Wait while charging"));
    batteryLevel = constrain(batteryLevel, 0, 100);
    delay(chargingTime);
} // end if

void chargeFullBattery(int missingPercentage){
    account -= missingPercentage;
    batteryLevel += missingPercentage;
    display.clear();
    display.print(F("Wait while charging"));
    batteryLevel = constrain(batteryLevel, 0, 100);
    delay(chargingTime);
} // end if

void chargeOnDebit(int percentage){
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
    delay(200);

    while ((buttonA.isPressed() == 0) && (buttonB.isPressed() == 0)){
    } // end while
    if (buttonA.isPressed() == 1){
        debit += missingAmount;
        account = 0;
        batteryLevel += percentage;
        display.clear();
        display.print(F("Wait while charging"));
        batteryLevel = constrain(batteryLevel, 0, 100);
        delay(chargingTime);
    } // end if

    if (buttonB.isPressed() == 1){
        chargingModeEntered = false;
        driveModeController = code1;
    } // end if
} // end void

void batteryLife(){
    unsigned long currentMillis = millis();
    unsigned int oneMinute = 60000;

    registerBatteryBelowFivePercent();
    registerTimeMotorIsRunning();
    registerHighSpeedData();
    
    if ((runningHours >= oneMinute) || (((currentMillis - runStartedAt) + runningHours) >= oneMinute)){ // Runs once each minute or when motor have ran for one minute straight
        calculateBatteryHealth();
        updateBatteryHealthEEPROM();  
    } // end if

    // Function to reduce batteryHealth if batteryFault occurs. Reduses batteryLife by 50%
    possibleProductionFault();

    // Function to do a service if batteryHealth is below 50% and car have enough money
    carNeedService();
    
    // Function to "change battery" when batteryHealth = 0;
    carNeedNewBattery(); 
} // end void

void registerBatteryBelowFivePercent(){
    if ((batteryHealth <= 5) && (incidentRegistered == false)){    // Record times batterylevel is less than or equal to 5%
        timesBelowFive += 1;
        incidentRegistered = true;
    } // end if

    else if (batteryHealth > 5){                                    // Code to prevent one incident recorded multiple times
        incidentRegistered = false;
    } // end else if
} // end void

void registerTimeMotorIsRunning(){
    unsigned long currentMillis = millis();
    if ((abs(iAmSpeed > 0)) && (motorRunning == false)){           // Record time when motors are running
        runStartedAt = currentMillis;
        motorRunning = true;
    } // end if

    else if (abs(iAmSpeed == 0) && (motorRunning == true)){        // Stop recording when motors are stopped
        runningHours += (currentMillis - runStartedAt);
        motorRunning = false;
    } // end else if
} // end void

void registerHighSpeedData(){
    unsigned long currentMillis = millis();

    if (iAmSpeed > maxSpeedLastMinute){                             // Record the maximum speed in this period
        maxSpeedLastMinute = iAmSpeed;
    } // end if
    
    if (iAmSpeed > (400 * 0,7) && (timerStarted == false)){        // Start recording time for motors going above 70% of absolute max speed
        aboveSeventyTimer = currentMillis;                          // when speed goes above 70%
        timerStarted = true;
    } // end if

    if (iAmSpeed < (400 * 0,7) && (timerStarted == true)){         // Stop recording time for motors going above 70% of absolute max speed
        timeNearMaxSpeed += (currentMillis - aboveSeventyTimer);    // when speed goes below 70% and add to total storage variable
        timerStarted = false;
    } // end if
} // end void

void calculateBatteryHealth(){
    unsigned long currentMillis = millis();
    unsigned int oneMinute = 60000;

    if (timerStarted == true){
        timeNearMaxSpeed += (currentMillis - aboveSeventyTimer);  
    } // end if

    lastMinuteAverageSpeed = (meassureDistance - minuteStartDistance) / oneMinute;                  // Store average speed
    lastMinuteMaxSpeed = maxSpeedLastMinute;                                                      // Store maxSpeed
    lastMinuteAboveSeventyPercent = timeNearMaxSpeed;                                               // Store time above 70% of absolute maximum speed
    
    minuteStartDistance = meassureDistance;                                                         // Reset variables for next minute running
    aboveSeventyTimer = 0;                                                      
    maxSpeedLastMinute = 0;
    runningHours = 0;
    runStartedAt = currentMillis;
    motorRunning = false;
    timerStarted = false;

    // Calculation to reduce batteryHealth based on avearage speed, max speed, time above 70% of max speed, times battery have been below 5% and time battery have been charged
    int maxSpeedEffect = constrain(lastMinuteMaxSpeed - 30, 0, 30);
    batteryHealth -= round(((lastMinuteAverageSpeed / 10) + (maxSpeedEffect / 10) + (lastMinuteAboveSeventyPercent / 2000) + timesBelowFive + timesCharged));
} // end void

void possibleProductionFault(){
    // Function to reduce batteryHealth if batteryFault occurs. Reduses batteryLife by 50%
    unsigned long currentMillis = millis();

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
} // end void

void carNeedService(){
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
} // end void

void carNeedNewBattery(){
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
        display.print(F("A = Change Battery"));
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
    batteryHealth = constrain(batteryHealth, 0, 100);
    if (batteryHealth != previousBatteryHealth){
            EEPROM.write(0,batteryHealth); // To reduce writing to EEPROM the value is only updated if value has changed
            previousBatteryHealth = batteryHealth;
        } // end if
} // end void

void wrongWayReverseAndTurn(){
    unsigned long currentMillis = millis();
    const int reverseTimerInterval = 10000;

    lineSensors.read(lineSensorValues); // Reads the current values for each of the line sensors

    /* Values Calculated by reding sensor values of track and directly over tape, and then found the limit inbetween to 
    make the function work. Might need to be changed when using another car. Since the working limits varied between 
    the sensors, we did not make a function to automaticly calculate good limit values. */
    int sensorOneLimit = 1000;
    int sensorTwoLimit = 500;
    int sensorThreeLimit = 500;
    int sensorFourLimit = 700;
    int sensorFiveLimit = 1000;
    

    /* Registered if we drive trough a crossection with a left turn. We had to limit the code not to 
    include automatic turning for crossection with right turn because of program storage space. */
    if((lineSensorValues[0] > 1300) && (lineSensorValues[1] > 900) && reverseTimerStarted == false){
        reverseTimerStarted = true;
        reverseTimer = currentMillis;
    } // end if

    // Limits time to turn after passing a crossection
    if (currentMillis - reverseTimer > reverseTimerInterval){
        reverseTimerStarted = false;
    } // end if

    if (reverseTimerStarted == true){
        if (trackIsLost == false){
            
            // Checks if all sensors are of the track
            if ((lineSensorValues[0] <= sensorOneLimit) && (lineSensorValues[1] <= sensorTwoLimit) && (lineSensorValues[2] <= sensorThreeLimit) && (lineSensorValues[3] <= sensorFourLimit) && (lineSensorValues[4] <= sensorFiveLimit)){
                motors.setSpeeds(200,200);
                delay(500);
                lineSensors.read(lineSensorValues);

                // Rechecks if any sensors have found the track
                if ((lineSensorValues[0] > sensorOneLimit) || (lineSensorValues[1] > sensorTwoLimit) || (lineSensorValues[2] > sensorThreeLimit) || (lineSensorValues[3] > sensorFourLimit) || (lineSensorValues[4] > sensorFiveLimit)){
                    previousDriveCase();
                } // end if

                else{
                    motors.setSpeeds(0,0);
                    delay(500);
                    trackIsLost = true;
                } // end if
            } // end if
        } // end if

        while (trackIsLost == true){
            reverseTimerStarted = false;
            lineSensors.read(lineSensorValues);
            if ((lineSensorValues[0] <= sensorOneLimit) && (lineSensorValues[4] <= sensorFiveLimit)){
                motors.setSpeeds(-200, -200);
            } // end if

            else if (lineSensorValues[0] > sensorOneLimit){
                motors.setSpeeds(0,0);

                for (int i = 0; i < 200; i++){
                    motors.setSpeeds(0,i);
                    delay(5);
                } // end for

                for (int i = 200; i > 0; i--){
                    motors.setSpeeds(0,i);
                    delay(5);
                } // end for
                trackIsLost = false;
                previousDriveCase();
            } // end else if
        } // end while
    } // end if
} // end void

void previousDriveCase(){
    if (lastGivenCase = 0){
        driveModeController = code1;
    } // end if
    else if (lastGivenCase = 1){
        driveModeController = code2;
    } // end else if
    else if (lastGivenCase = 2){
        driveModeController = code3;
    } // end else if
    else if (lastGivenCase = 3){
        driveModeController = chargingStation;
    } // end else if
} // end void

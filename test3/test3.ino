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
uint8_t batteryLevel = 0;
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
#define taxiPassenger  671389550//dette er hva ladestasjonen sender ut hvis det er passasjerer som venter.
const long RECV_PIN = A4;
IRrecv irrecv(RECV_PIN);
unsigned long irNum;
unsigned long driveModeController;
unsigned long taxiModeController;
unsigned long changeSpeedController;
/////////////////////////////


// Variables for hiddenFeatureForCharging()
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
unsigned long previousLowBatteryMillis = 0;
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

//Variables for wrongWayReverseAndTurn()
bool trackIsLost = false;

///////// TEST VARIABLES ////
byte lastGivenCase;



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
    display.clear();
    display.setLayout21x8();
    display.gotoXY(2,1);
    display.print(F("Battery is empty"));
    display.gotoXY(6,4);
    display.print(F("GAME OVER"));
    display.gotoXY(0,6);
    display.print(F("Game balance:"));
    display.gotoXY(15,6);
    display.print(account);
    while (batteryLevel == 0){
    }
} // end loop

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
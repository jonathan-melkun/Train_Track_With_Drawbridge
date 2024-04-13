#include <SafeString.h>
#include <loopTimer.h>
#include <BufferedOutput.h>
#include <millisDelay.h>
#include <SafeStringReader.h>
#include <AccelStepper.h>
#define LEFT 1
#define STRAIGHT 0

// PIN DECLARATIONS
int dirPin      = 2;
int trigPin     = 3;
int stepperPin1 = 5;
int stepperPin2 = 6;
int stepperPin3 = 7;
int stepperPin4 = 8;
int ledPin      = 9;
int relayPin    = 10;
int hallPin1    = 11;
int hallPin2    = 12;
int hallPin3    = 13;

// TRICKLE CHARGE
bool dir = LEFT;

// STEPPER MOTOR
const int stepsPerRevolution  = 2038;  // number of steps per 1 full revolution
const int stepperMaxSpeed     = 600;   // speed in RPM
const int stepperAcceleration = 300;   
const int stepperDownPosition   = 3000;
AccelStepper stepper(4, stepperPin1, stepperPin3, stepperPin2, stepperPin4);

//BufferedOutput to avoid blocking in Serial.print
createBufferedOutput(bufferedOut, 80, DROP_UNTIL_EMPTY);

//Blocks the sequences from being triggered repeatedly
int JUNCTION_BLOCKING_TIME = 9000; //ms
int BRIDGE_BLOCKING_TIME = 1000;
int TRAIN_BRIDGE_WAIT = 8000;
int BRIDGE_UP_BLOCKING_TIME = 1000;

bool ledOn = false; // keep track of the led state
bool switchingStarted = false; // keep track of trickle charge state
millisDelay ledDelay;
millisDelay bridgeDelay;
millisDelay bridgeUpDelay;
millisDelay junctionTrigTimer;
millisDelay trainStartTimer;
millisDelay switchBlockOnDelay;

void setup() {
  Serial.begin(9600);
  bufferedOut.connect(Serial);  // connect bufferedOut to Serial
  bufferedOut.println("Initializing");

  stepper.setMaxSpeed(stepperMaxSpeed);
  stepper.setAcceleration(stepperAcceleration);
  pinMode(dirPin, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(hallPin1, INPUT_PULLUP);
  pinMode(hallPin2, INPUT_PULLUP);
  pinMode(hallPin3, INPUT_PULLUP);
  
  digitalWrite(relayPin, HIGH);
  digitalWrite(trigPin, HIGH);
  digitalWrite(dirPin, dir);
  digitalWrite(trigPin, LOW);
  delay(100);
  digitalWrite(trigPin, HIGH);
  ledDelay.start(1000); // start the ledDelay, toggle every 1000ms, heartbeat
}

void blinkLed() {
  if (ledDelay.justFinished()) {   // check if delay has timed out
    ledDelay.repeat(); // start delay again without drift
    ledOn = !ledOn;     // toggle the led
    //bufferedOut.println("LED Toggling");
    digitalWrite(ledPin, ledOn ? HIGH : LOW); // turn led on/off
  } // else nothing to do this call just return, quickly
}

void switchJunction() {
  if (!switchBlockOnDelay.isRunning()) {
    if (digitalRead(hallPin1) == LOW) {
      switchingStarted=true;
      //bufferedOut.println("Starting Switch");
      switchBlockOnDelay.start(JUNCTION_BLOCKING_TIME);
    }
  }
  if (switchingStarted) { // start one now
    switchingStarted = false;
    dir = !dir;
    digitalWrite(dirPin, dir);
    digitalWrite(trigPin, LOW);
    // start delay to turn off pin
    junctionTrigTimer.start(100);
  }
  if (junctionTrigTimer.justFinished()) {
    digitalWrite(trigPin, HIGH);
    //bufferedOut.println("Stopping Switch");
  }
}

void lowerBridge(){
  if (!bridgeDelay.isRunning()){
    if (digitalRead(hallPin2) == LOW) {
      digitalWrite(relayPin, LOW);
      //bufferedOut.println("Stopping Train");
      stepper.moveTo(stepperDownPosition);
      //bufferedOut.println("Lowering Bridge!");
      trainStartTimer.start(TRAIN_BRIDGE_WAIT);
      bridgeDelay.start(BRIDGE_BLOCKING_TIME);
    }
  }
  if (trainStartTimer.justFinished()) {
    digitalWrite(relayPin, HIGH);
    //bufferedOut.println("Starting Train");
  }
}

void raiseBridge(){
  if (!bridgeUpDelay.isRunning()){
    if (digitalRead(hallPin3) == LOW){
      stepper.moveTo(0);
      //bufferedOut.println("Raising Bridge!");
      bridgeUpDelay.start(BRIDGE_UP_BLOCKING_TIME);
  }
  }
}

void loop() {
  bufferedOut.nextByteOut(); // call at least once per loop to release chars
  //loopTimer.check(bufferedOut); // send loop timer output to the bufferedOut
  blinkLed();
  switchBlockOnDelay.justFinished();
  switchJunction(); 
  bridgeDelay.justFinished();
  lowerBridge();
  bridgeUpDelay.justFinished();
  raiseBridge();
  stepper.run();
}

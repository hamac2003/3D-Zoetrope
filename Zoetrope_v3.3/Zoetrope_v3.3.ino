/*
  Author: Harrison McIntyre
  Youtube Channel: https://www.youtube.com/c/HarrisonMcIntyre
  Last Updated: 6.25.2021
  
  Credit for the "MillisDelay" library code here: https://www.forward.com.au/pfod/ArduinoProgramming/TimingDelaysInArduino.html

*/

#include <millisDelay.h>

// L298N Control Pins
#define directionPin1 9
#define directionPin2 10
#define speedPin 11

// Encoder Pin
#define encoderPin 2

// Light Triggering Pin
#define npnLine 8

// External Control Pins
#define switchPin1 3
#define switchPin2 4

// Various Timers for Checking External Controls
millisDelay switchDelay;
millisDelay potDelay;
millisDelay checkSpeedDelay;

// Some sort of delay after the light is triggered (I honestly don't remember what it does)
unsigned long pulseTimer = 0;

// The light's current state
int lightState = 0;
// The light's previous state
int prevLightState = 1;
// Wheter or not to strobe the light (turn it on then back off again quickly)
boolean strobe = false;
// Total recorded light pulses
unsigned long totalPulses = 0;

// The gear ratio for the motor
double gearRatio = 62.4;
// Number of encoder pulses per revolution of the motor
double ppr = 11;

// External switch state
int switchState = 1;
// External dial val
double dialVal = 0.0;
// Previous external dial val
double prevDialVal = 0.0;

// Adjustment delay for drift compensation on the animation
int adjustmentVal = 0;
// Number of total frames
double framesPerRev = 20.0;

// Used to calculate the RPM of the motor
unsigned long rpmTime = 1;
// The current rotations per minute of the motor
double rpm = 0.0;
// The Desired rotations per minute of the motor
double setRpm = 0.0;

// Variables used in closed loop control of the motor's RPM
double difference = 0.0;
double gain = 0.05;

// Used for debugging purposes
unsigned long debugVal = 0;

// Used to calculate when to strobe the light next
unsigned long rawStrobeTime = 0;

void setup() {
  // Initialize the necessary pins
  pinMode(directionPin1, OUTPUT);
  pinMode(directionPin2, OUTPUT);
  pinMode(speedPin, OUTPUT);
  pinMode(npnLine, OUTPUT);
  
  pinMode(switchPin1, INPUT_PULLUP);
  pinMode(switchPin2, INPUT_PULLUP);

  // Start the timers that we use to check various controls and machine states
  switchDelay.start(500);
  potDelay.start(700);
  checkSpeedDelay.start(1);

  // Attach an interrupt to the encoder pin
  attachInterrupt(digitalPinToInterrupt(encoderPin), increment, FALLING);

  // Start serial communication
  //Serial.begin(9600);

  // Wait (because why not)
  delay(1000);

  // Set some initial states for the L298N control pins
  digitalWrite(directionPin1, HIGH);
  digitalWrite(directionPin2, LOW);
  analogWrite(speedPin, 240);
}


// Function for pulsing the overhead light
void pulseLight(unsigned long val) {
  // If commanded to strobe, do
  if (strobe) {
    // Only strobe once
    strobe = false;
    // Turn the light on
    lightState = 1;
    // Store the current time in microseconds
    pulseTimer = micros();
  }
  // If the desired time has elapsed turn the light off
  if (micros() - pulseTimer >= val) {
    lightState = 0;
  }
  // Change the light state only if it has changed
  if (lightState != prevLightState) {
    digitalWrite(npnLine, lightState);
    prevLightState = lightState;
  }
}

// Function for checking the external switch
void checkSwitch() {
  // If it's time to check the switch, do
  if (switchDelay.justFinished()) {
    // Reset the timer
    switchDelay.repeat();
    // Read the switch state
    if (!digitalRead(switchPin1)) {
      switchState = -1;
    }else if (!digitalRead(switchPin2)) {
      switchState = 0;
    }else {
      switchState = 1;
    }
  }
  // Update the motor speed
  analogWrite(speedPin, dialVal);
}

// Function for checking the external dial
void checkDial() {
  // If it's time to check the dial, do
  if (potDelay.justFinished()) {
    // Reset the timer
    potDelay.repeat();
    // Read the dial
    adjustmentVal = (analogRead(A3) / 1024.0 * 400) - 200;
    if (adjustmentVal >= -5 && adjustmentVal <= 5) {
      adjustmentVal = 0;
    }
  }
}

// Function for closed loop control of the motor speed
void checkSpeed() {
  // If it's time to check the motor speed, do
  if (checkSpeedDelay.justFinished()) {
    // Restart the timer
    checkSpeedDelay.repeat();
    // Check the motor speed and make necessary adjustments to motor output
    difference = (setRpm - rpm) / 5;
    difference = constrain(difference, -1, 1);
    dialVal = dialVal + difference*gain;
  }
}

void loop() {
  // Pulse the light for 1000 microseconds (1 millisecond)
  pulseLight(1000);
  
  // Check the various external controls and motor speed
  checkSwitch();
  checkDial();
  checkSpeed();

  // Calculate the motor RPM
  rpm = 1000000/(debugVal*ppr*gearRatio)*60;

  // Various display modes based upon external switch input
  if (switchState == 1) {
    totalPulses = 0;
    setRpm = 84;

    // If it's time to strobe the light, do
    if (micros() - rawStrobeTime >= 35755 + adjustmentVal) {
      strobe = true;
      rawStrobeTime = micros();
    }

  }else if (switchState == 0) {
    totalPulses = 0;
    setRpm = 90;

    // If it's time to strobe the light, do
    if (micros() - rawStrobeTime >= 33270 + adjustmentVal) {
      strobe = true;
      rawStrobeTime = micros();
    }
  }else if (switchState == -1) {
    totalPulses = 0;
    dialVal = 150;

    // Turn the light off
    digitalWrite(npnLine, LOW);
  }
}

// Function to increment totalPulses every time the encoder pulses
void increment() {
  debugVal = (micros() - rpmTime);
  rpmTime = micros();
  totalPulses++;
}

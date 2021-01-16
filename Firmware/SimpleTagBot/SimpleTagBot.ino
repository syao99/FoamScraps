/*
	Tagbot prototype firmware.
	Quick, simple flywheel blaster. Features: Rev speed control, rev switch and firing switch logic, semi/full firing modes, brushless motors.
	Pinouts set up for Arduino Pro Mini.
	WARNING: This code is not yet fully tested, use at own risk!
	Todo: Add second stage pins with staggered spinup to reduce initial power draw. Add brushed motor modes maybe.

	Copyright (C) 2020 Input Eater Creations.
	Licensed under The MIT License.
*/

# include <Servo.h>

// Inputs
const int revSpeedPin = A3;
const int revSwitchPin = A2;
const int trigSwitchPin = A1;
const int selectorSwitchPin = A0;

// Outputs
const int escPin = 12;
const int solenoidPin = 11;
// const int pwmStageOnePin = 10; // Reserved for brushed motors

// User Params
const int minWheelSpeed = 1000;
const int maxWheelSpeed = 2000;
const int dps = 10;
const float dwellPercentage = 0.5;
// const int deadzone = 10;

// END OF USER PARAMS ________________________________________________________________

// Auto-Calculated Params - all units milliseconds
const float fireCycleTime = 1000 / dps;
const float dwellTime = dwellPercentage * fireCycleTime;
const float offTime = fireCycleTime - dwellTime;

// State
int currentSpeed = 0;
unsigned long firingStartTime = 0;
unsigned long currentFiringTime = 0;
bool previousIsFiring = false;
Servo esc;

// Required Functions

void setup() {
	pinMode(revSpeedPin, INPUT);
	pinMode(revSwitchPin, INPUT_PULLUP);
	pinMode(trigSwitchPin, INPUT_PULLUP);
	pinMode(selectorSwitchPin, INPUT_PULLUP);

	pinMode(escPin, OUTPUT);
	pinMode(solenoidPin, OUTPUT);

	esc.attach(escPin);
	//Serial.begin(9600);
	//Serial.println("start");
}

void loop() {
	esc.writeMicroseconds(getRevLogic());
	digitalWrite(solenoidPin, getFiringLogic());
	previousIsFiring = isFiring();
}

// Impure Functions

void resetFiringTimer() {
	currentFiringTime = 0;
	firingStartTime = millis();
}

// Pure Functions

int getOutputSpeed(int inputValue) {
	/* Deadzone mode for analog revswitches, not used.
	if (inputValue < deadzone) {
		return 0;
	} else {
		return map(inputValue, deadzone, 1023, minWheelSpeed, maxWheelSpeed); }
	*/
	return map(inputValue, 0, 1023, minWheelSpeed, maxWheelSpeed);
}

bool isRevving() {
	return digitalRead(revSwitchPin) == LOW;
}

bool isFiring() {
	return digitalRead(trigSwitchPin) == LOW;
}

bool isAltFiringMode() {
	return digitalRead(selectorSwitchPin) == LOW;
}

unsigned long getCurrentFiringTimerTime() {
	return millis() - firingStartTime;
}

// Complex Pure Functions

bool getFiringLogic() { // Return bool controlling firing.
	if (isFiring() && !previousIsFiring) { // If the firing switch just got pressed.
		resetFiringTimer();
		return true;
	}
	else if (isFiring() && previousIsFiring) { // If the fs has been held down.
		currentFiringTime = getCurrentFiringTimerTime();
		if (currentFiringTime < dwellTime) {
			return true;
		}
		else if (currentFiringTime < fireCycleTime) {
			return false;
		}
		else { // If the fs has been held longer than one full firing cycle
			if (isAltFiringMode()) { // Reset firing timer if in FA mode
				resetFiringTimer();
				return true;
			}
			return false;
		}
	}
	else if (!isFiring()) { // If the fs isn't being pressed.
		currentFiringTime = getCurrentFiringTimerTime();
		if (currentFiringTime < dwellTime) { // Keep solenoid forward until dwelltime has passed.
			return true;
		}
	}
	return false;
}

int getRevLogic() { // Return int with the rev speed.
	if (isRevving() || isFiring()) { // If either or both rev/firing switches are pressed, return the rev speed.
		return getOutputSpeed(analogRead(revSpeedPin));
	}
	return 0;
}

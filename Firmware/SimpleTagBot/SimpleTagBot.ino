// 
// Quick, simple flywheel blaster.
// Controls: Rev speed, Rev switch, Firing switch.
// Outputs: Flywheel spin, Solenoid firing.
// Pinouts set up for Arduino Pro Mini.
// Todo: Add second stage pins with staggered spinup to reduce initial power draw.

# include <Servo.h>

// Inputs
const int revSpeedPin = A0;
const int revSwitchPin = 12;
const int trigSwitchPin = 11;
const int selectorSwitchPin = 10;

// Outputs
const int motorPin = 8;
const int solenoidPin = 9;

// Params
const int minWheelSpeed = 1000;
const int maxWheelSpeed = 2000;
const int dps = 10;
const float dwellPercentage = 0.5f;
// const int deadzone = 10;

// End of User Params ________________________________________________________________

// Auto-Calculated Params - all units milliseconds
const float fireCycleTime = 1000 / dps;
const float dwellTime = dwellPercentage * fireCycleTime;
const float offTime = fireCycleTime - dwellTime;

// State
int currentSpeed = 0;
unsigned long firingStartTime = 0;
bool previousIsFiring = false;
Servo esc;

// Required Functions

void setup() {
	pinMode(revSpeedPin, INPUT);
	pinMode(revSwitchPin, INPUT_PULLUP);
	pinMode(trigSwitchPin, INPUT_PULLUP);
	pinMode(selectorSwitchPin, INPUT_PULLUP);

	pinMode(motorPin, OUTPUT);
	pinMode(solenoidPin, OUTPUT);

	esc.attach(motorPin);
	//Serial.begin(9600);
	//Serial.println("start");
}

void loop() {
	esc.writeMicroseconds(getRevLogic());
	digitalWrite(solenoidPin, getFiringLogic());
	previousIsFiring = isFiring();
}

// Non-pure Functions
void resetFiringTimer() {
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
	} else if (isFiring() && previousIsFiring) { // If the fs has been held down.
		unsigned long currentFiringTime = getCurrentFiringTimerTime();
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
		unsigned long currentFiringTime = getCurrentFiringTimerTime();
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
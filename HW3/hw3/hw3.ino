#include <CapacitiveSensor.h>

/*
 * Setup:
 * Import CapacitiveSensor Library via Sketch->Add Library.
 * Hook up buttons to d2-3. One side ground, other side digital pin.
 * Put 10M resistor between d8 and d9-10. Connect Foil/wire to d9-10.
 * Hook up FSR to a0 and Vcc.
 *
 * Note that FSR currently will always output ACCELERATE or BRAKE.
 * If Processing is running, this means either the Up or Down arrow is being held.
 */

/*
 * Buttons
 * NITRO, ITEM
 */
const int numButtons = 2;
const int buttonPins[numButtons] = {2, 3};
const String buttonPressString[numButtons] = {"NITRO","ITEM"};
const String buttonReleaseString[numButtons] = {"NITRO_R","ITEM_R"};

/*
 * Debouncing State
 * Based on Debounce tutorial.
 */
const long debounceDelay = 50; // debounce time threshold

int buttonState[numButtons];
int previousReading[numButtons] = {HIGH, HIGH};
long lastDebounceTime[numButtons] = {0, 0};

/*
 * Capacitive Sensors
 * LEFT, RIGHT
 */
enum turnState {
  LEFT,
  RIGHT,
  STRAIGHT
};

const int numCS = 2;
const long csThreshold = 175;

CapacitiveSensor cs[4] = {CapacitiveSensor(8,9), CapacitiveSensor(8,10), CapacitiveSensor(8,11), CapacitiveSensor(8,12)};
// 10M resistor between pins 8 & 9, pin 9 is sensor/receive pin, add a wire and or foil if desired. Same for 10-12.
boolean csActive[4] = {false, false, false, false};
turnState csTurnState = STRAIGHT;
int turnRateState = 0;

/*
 * FSR
 * ACCELERATE, BRAKE, NEUTRAL
 */
enum speedState {
  ACCELERATE,
  BRAKE,
  NEUTRAL
};
const int fsrPin = A0;
const float fsrBrakeThreshold = 1.50;
const float fsrAccelerateThreshold = 4.00;

speedState previousSpeedState = NEUTRAL;

/*
 * Accelerometer
 */
const int numAxes = 3;
const int axesPins[numAxes] = {A1, A2, A3};
const int driftUpperThreshold = 350;
const int driftLowerThreshold = 310;
int axesValues[numAxes];

void setup() {
  Serial.begin(9600);
  // Buttons
  for (int i = 0; i < numButtons; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  
  // Capacitive Sensors
  for (int i = 0; i < numCS; i++) {
    cs[i].reset_CS_AutoCal(); //calibrate
    cs[i].set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate
  }
  
  pinMode(fsrPin, INPUT);
  for (int i = 0; i < numAxes; i++) {
    pinMode(axesPins[i], INPUT);
  }
  
}

void loop() {
  // Buttons
  for (int i = 0; i < numButtons; i++) {    
    int reading = digitalRead(buttonPins[i]);
    
    if (reading != previousReading[i]) {
      // Potential state change detected. Set time marker.
      lastDebounceTime[i] = millis();
    }
    
    if (millis() - lastDebounceTime[i] > debounceDelay) {
    // Debounce time threshold passed
      if (reading != buttonState[i]) {
        // Actual State change detected.
        buttonState[i] = reading;
        if (reading == HIGH) {
          // Button released.
          Serial.println(buttonReleaseString[i]);
        }
      }
      
    }

    // Update previous reading.
    previousReading[i] = reading;
    
    if (buttonState[i] == LOW) {
      // Button held.
      Serial.println(buttonPressString[i]);      
    }
  }
  
  // Capacitive Sensors
//  for (int i = 0; i < numCS; i++) {
//    long reading =  cs[i].capacitiveSensor(30);
//    Serial.print(reading);
//    Serial.print(" ");
//    csActive[i] = reading > csThreshold;
//  }
  long reading0 =  cs[0].capacitiveSensor(30);
  long reading1 =  cs[1].capacitiveSensor(30);
//  Serial.print(reading0);
//  Serial.print(" ");
//  Serial.print(reading1);
//  Serial.println("");

  csActive[0] = reading0 > csThreshold && reading0 > reading1;
  csActive[1] = reading1 > csThreshold && reading1 > reading0;
  updateCSState();
  
  // FSR
  // From ReadAnalogVoltage Tutorial
  int sensorValue = analogRead(fsrPin);
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  float voltage = sensorValue * (5.0 / 1023.0);
//  Serial.println(voltage);
  updateSpeedState(voltage);
    
  // Accelerometer
  for (int i = 0; i < numAxes; i++) {
    axesValues[i] = analogRead(axesPins[i]);
  }
  Serial.println(axesValues[0]);
//  Serial.print(", ");
//  Serial.print(axesValues[1]);
//  Serial.print(", ");
//  Serial.println(axesValues[2]);
  if (axesValues[0] > driftUpperThreshold || axesValues[0] < driftLowerThreshold) {
    Serial.println("DRIFT");
  } else {
    Serial.println("DRIFT_R");
  }
//  delay(1000);
}

void updateCSState() {
  turnState newState;
  if (csActive[0] && csActive[1]) {
    newState = STRAIGHT;
  } else if (csActive[2] && csActive[3]) {
    newState = STRAIGHT;
  } else if (csActive[0] || csActive[2]) {
    newState = RIGHT;
  } else if (csActive[1] || csActive[3]) {
    newState = LEFT;
  } else {
    newState = STRAIGHT;
  }
  
  boolean stateChange = newState != csTurnState;
  
  if (stateChange) {
    switch (csTurnState) {
      case LEFT:
        Serial.println("LEFT_R");
        break;
      case RIGHT:
        Serial.println("RIGHT_R");
        break;
      case STRAIGHT:
        break;
    }
  }
  
  csTurnState = newState;
  turnRateState = (turnRateState + 1) % 4;
  if (turnRateState == 0) {
    switch (csTurnState) {
        case LEFT:
          Serial.println("LEFT");
          break;
        case RIGHT:
          Serial.println("RIGHT");
          break;
        case STRAIGHT:
          break;
    }
  }
}

void updateSpeedState(float voltage) {
  speedState newState;
  if (voltage > fsrAccelerateThreshold) {
    newState = ACCELERATE;
  } else if (voltage < fsrBrakeThreshold) {
    newState = BRAKE;
  } else {
    newState = NEUTRAL;
  }
  
  boolean stateChange = newState != previousSpeedState;
  
  if (stateChange) {
    switch (previousSpeedState) {
      case ACCELERATE:
        Serial.println("ACCELERATE_R");
        break;
      case BRAKE:
        Serial.println("BRAKE_R");
        break;
      case NEUTRAL:
        break;
    }
  }
  
  previousSpeedState = newState;
  switch(newState) {
    case ACCELERATE:
      Serial.println("ACCELERATE");
      break;
    case BRAKE:
      Serial.println("BRAKE");
      break;
    case NEUTRAL:
      break;
  }
   
}

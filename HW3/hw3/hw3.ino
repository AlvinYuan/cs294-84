#include <CapacitiveSensor.h>

/*
 * Setup:
 * Import CapacitiveSensor Library via Sketch->Add Library.
 * Hook up buttons to d2-6. One side ground, other side digital pin.
 * Put 10M resistor between d8 and d9. Connect Foil/wire to d9.
 * Hook up FSR to a0 and Vcc.
 *
 * Note that FSR currently will always output ACCELERATE or BRAKE.
 * If Processing is running, this means either the Up or Down arrow is being held.
 *
 * Note that the capacitive sensor threshold is currently 10,000.
 * This can only be reached when touching th foil/wire.
 */

/*
 * Buttons
 */
const int numButtons = 6;
const int buttonPins[numButtons] = {2, 3, 4, 5, 6, 7};
const String buttonPressString[numButtons] = {"NITRO","BACKVIEW","DRIFT","ITEM", "5","6"};
const String buttonReleaseString[numButtons] = {"NITRO_R","BACKVIEW_R","DRIFT_R","ITEM_R", "5_R", "6_R"};

/*
 * Debouncing State
 * Based on Debounce tutorial.
 */
const long debounceDelay = 50; // debounce time threshold

int buttonState[numButtons];
int previousReading[numButtons] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
long lastDebounceTime[numButtons] = {0, 0, 0, 0, 0, 0};

/*
 * Capacitive Sensors
 */
const int numCS = 1;
const String csActiveString[numCS] = {"LEFT"};
const String csInactiveString[numCS] = {"LEFT_I"};
const long csThreshold = 10000;

CapacitiveSensor cs[numCS] = {CapacitiveSensor(8,9)}; // 10M resistor between pins 8 & 9, pin 9 is sensor/receive pin, add a wire and or foil if desired
boolean csActive[numCS] = {false};

/*
 * FSR
 * TODO: Change to not be boolean on/off? Use PWD keypresses?
 */
const float fsrThreshold = 4.00;
const String fsrActiveString = "ACCELERATE";
const String fsrInactiveString = "BRAKE";

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
  for (int i = 0; i < numCS; i++) {
    long reading =  cs[i].capacitiveSensor(30);
    if (reading > csThreshold) {
      Serial.println(csActiveString[i]);
    } else if (reading < csThreshold && csActive[i]) {
      Serial.println(csInactiveString[i]);
    }
    csActive[i] = reading > csThreshold;
  }
  
  // FSR
  // From ReadAnalogVoltage Tutorial
  // read the input on analog pin 0:
  int sensorValue = analogRead(A0);
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  float voltage = sensorValue * (5.0 / 1023.0);
  if (voltage > fsrThreshold) {
    Serial.println(fsrActiveString);
  } else {
    Serial.println(fsrInactiveString);
  }
    
}

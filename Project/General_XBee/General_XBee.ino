/*
 * General Device Code with XBee
 *
 * Setup:
 * XBee - Arduino
 * VSS  - GND
 * VCC  - 3V
 * DOUT - RX
 * DIN  - TX
 *
 * Danger Button
 * One end to d2
 * Other end to GND
 *
 * Function:
 * Pressing Danger Button should broadcast a danger message.
 * Receiving a danger message should turn on LED for 3 seconds.
 */
 
const int dangerButtonPin = 2;
const int dangerLEDPin = 13;
long lastDangerMessageReceivedTime = 0;
const int phoneInput = A0; // ghetto ass input
const int analogThresh = 200;

/*
 * Debouncing State
 * Based on Debounce tutorial.
 */
const long debounceDelay = 50; // debounce time threshold
int dangerButtonState;
int previousDangerButtonReading = HIGH;
long lastDebounceTime = 0;

// for audio checking

int currentValue = 0;
int prevValue = 0;


void setup() {
  Serial.begin(9600);

  pinMode(dangerButtonPin, INPUT_PULLUP);
  pinMode(dangerLEDPin, OUTPUT);


}

void loop() {
  // Danger LED
  if(Serial.available()){
    char getData = Serial.read();
    if(getData == 'D'){
      lastDangerMessageReceivedTime = millis();
    }
  }
  
  if (millis() - lastDangerMessageReceivedTime > 3000) {
    digitalWrite(dangerLEDPin, LOW);
  } else {
     digitalWrite(dangerLEDPin, HIGH);
  }

  // Danger Button
  int dangerButtonReading = digitalRead(dangerButtonPin);
    
  if (dangerButtonReading != previousDangerButtonReading) {
    // Potential state change detected. Set time marker.
    lastDebounceTime = millis();
  }
    
  if (millis() - lastDebounceTime > debounceDelay) {
    // Debounce time threshold passed
    if (dangerButtonReading != dangerButtonState) {
      // Actual State change detected.
      dangerButtonState = dangerButtonReading;
      if (dangerButtonReading == HIGH) {
        // Button released.
        Serial.print("D");
      }
    }    
  }
  previousDangerButtonReading = dangerButtonReading;
  
  prevValue = currentValue;
  currentValue = analogRead(phoneInput);
  if ( currentValue ==0 && prevValue != 0){
    Serial.print("X");
  }
    
    
    
  
}

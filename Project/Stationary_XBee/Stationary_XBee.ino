/*
 * Stationary Device Code with XBee
 *
 * Setup:
 * XBee - Arduino
 * VSS  - GND
 * VCC  - 3V
 * DOUT - RX
 * DIN  - TX
 *
 * Function:
 * If received a danger message, turn on LED.
 * Receiving a danger message will cause this to broadcast
 * danger messages every 5 seconds.
 */

int dangerMessageReceivedLEDPin = 13;
boolean dangerMessageReceived = false;
long lastDangerMessageSentTime = 0;

void setup() {
  Serial.begin(9600);
  pinMode(dangerMessageReceivedLEDPin, OUTPUT);
  digitalWrite(dangerMessageReceivedLEDPin, LOW);
}

void loop() {
  // Danger LED
  if(Serial.available()){
    char getData = Serial.read();
    if(getData == 'D'){
      dangerMessageReceived = true;
      digitalWrite(dangerMessageReceivedLEDPin, HIGH);
    }
  }
  
  if (dangerMessageReceived && millis() - lastDangerMessageSentTime > 5000) {
    Serial.print("D");
    lastDangerMessageSentTime = millis();
  }
}

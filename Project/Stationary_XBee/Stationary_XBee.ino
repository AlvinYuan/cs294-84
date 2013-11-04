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

int dangerMessageReceived2LEDPin = 9;
int dangerMessageReceivedLEDPin = 13;
boolean dangerMessageReceived = false;
long lastDangerMessageSentTime = 0;

void setup() {
  Serial.begin(9600);
  pinMode(dangerMessageReceivedLEDPin, OUTPUT);
  pinMode(dangerMessageReceived2LEDPin, OUTPUT);
  digitalWrite(dangerMessageReceivedLEDPin, LOW);
  digitalWrite(dangerMessageReceived2LEDPin, LOW);
}

void loop() {
  // Danger LED
  if(Serial.available()){
    char getData = Serial.read();
    if(getData == 'D'){
//      Serial.println("i gotchu yabish");
      dangerMessageReceived = true;
      digitalWrite(dangerMessageReceivedLEDPin, HIGH);
      digitalWrite(dangerMessageReceived2LEDPin, HIGH);
      delay(333);
      digitalWrite(dangerMessageReceived2LEDPin, LOW);
      delay(333);
      digitalWrite(dangerMessageReceived2LEDPin, HIGH);
      delay(333);
      digitalWrite(dangerMessageReceived2LEDPin, LOW);
      delay(333);
      
    }
    
    else if(getData == 'X'){
      dangerMessageReceived = true;
      digitalWrite(dangerMessageReceivedLEDPin, HIGH);
      digitalWrite(dangerMessageReceived2LEDPin, HIGH);
      delay(1000);
      digitalWrite(dangerMessageReceived2LEDPin, LOW);
      delay(333);
    }
  }
  
  if (dangerMessageReceived && millis() - lastDangerMessageSentTime > 5000) {
    Serial.print("D");
    lastDangerMessageSentTime = millis();
  }
  
  

}

#include <AudioSerial.h>

/* Audio Serial Test
 * Communicate between phone and microcontroller.
 * Partner android app, ServoControl at: https://github.com/AlvinYuan/cs294-84
 * 
 * Setup: Phone -> Arduino
 * Left AND Right Channel Stereo - 4k Ohm - GND
 * Left OR Right Channel Stereo - Op Amp + Input
 * Low Voltage (above phone bias) - Op Amp - Input (10k pot will help)
 * Op Amp Output - 100 Ohm - A0 pin
 * VCC and GND to Op Amp
 * GND to Audio Jack GND
 *
 * Setup: Arduino -> Phone 
 * Voltage Divider (1/2 factor) between D3 and GND
 * Voltage Divider Output - Audio Jack Mic
 * Audio Jack Mic - 10k Ohm - GND
*/

#include <AudioSerial.h>

boolean debug;
// Open audio serial communication on pin A0
// Change baudrate to match your app (may be 20)
AudioSerial audioserial(A0,300);

const int micOutputPin = 3;
int previousValue = 0;
int count = 0;

const int debugPin = 4;

int incomingByte = 0;

void setup(){
//  Serial.begin(200);
 Serial.begin(57600);
 Serial.println("hello"); 
 pinMode(micOutputPin, OUTPUT); 
 digitalWrite(micOutputPin,LOW);
 pinMode(debugPin, INPUT_PULLUP);
 
 pinMode(13, OUTPUT);
 digitalWrite(13, LOW);
}

void loop(){
  debug = digitalRead(debugPin) == LOW;
  if (!debug) {
    
    audioserial.run(); 
    // Read one byte
    char receivebyte=audioserial.read();
    if(receivebyte>-1){
      Serial.println();
      Serial.println(receivebyte);
      Serial.println();
    } 
    
    
  } else {
    // Some Test Code
    delay(audioserial.getDelay() / 3);
    count=(count + 1) % 3;
    int value = analogRead(A0);
    if (previousValue == 0 && value == 0) {
      if (count == 0) {
        Serial.println(value);
      }
    } else {
      Serial.println(value);
      count = 0;
    }
    previousValue = value;

    // Other Test Code
//    int value = digitalRead(A0) == HIGH;
//    Serial.println(value);
//    if (Serial.available()) {
//      digitalWrite(13, HIGH);
//    }
//    if (mySerial.available()) {
//      Serial.println("available");
////      Serial.write(mySerial.read());
//    } else {
//      Serial.println("not available");
//    }
//    delay(1000);
  }
}

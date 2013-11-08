#include <AudioSerial.h>

/* ServoControl
*  Receive servo position from audio serial
* Arduino Pins:
* A0 on op amp output
* D2 output to voltage divider (1/2 factor) to capacitor to mic input
* GND and VCC to audio jack circuit
* More circuitry for audio jack.
*/

#include <AudioSerial.h>
#include <Servo.h> 

// Open audio serial communication on pin A0 at 20 bits per seconds
// with starting byte 138 and voltage threshold 30
AudioSerial audioserial(A0,20,138,30);

const int micOutputPin = 2;
int previousValue = 0;
int count = 0;

int incomingByte = 0;
void setup(){
 Serial.begin(9600);
 Serial.println("hello"); 
 pinMode(micOutputPin, OUTPUT); 
 digitalWrite(micOutputPin,LOW);
}

void loop(){
  audioserial.run(); 
  // Read one byte
  char receivebyte=audioserial.read();
  if(receivebyte>-1){
    Serial.println();
    Serial.println(receivebyte);
    Serial.println();
  } 
  
  // send bytes through the microphone
  if (Serial.available() > 0) {
          // read the incoming byte:
          incomingByte = Serial.read();
          
          if (incomingByte == 'a'){
           digitalWrite(micOutputPin,HIGH);
           Serial.println("you've received a danger message! correct message");
          }else{            
            digitalWrite(micOutputPin,LOW);
          }

  }

// Some Test Code
//    delay(audioserial.getDelay() / 3);
//  count=(count + 1) % 3;
//  int value = analogRead(A0);
//  if (previousValue == 0 && value == 0) {
//    if (count == 0) {
//      Serial.println(value);
//    }
//  } else {
//    Serial.println(value);
//    count = 0;
//  }
//  previousValue = value;

// Other Test Code
//  int value = analogRead(A0);
//  Serial.println(value);
}

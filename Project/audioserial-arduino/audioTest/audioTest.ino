#include <AudioSerial.h>

/* ServoControl
*  Receive servo position from audio serial
*/

#include <AudioSerial.h>
#include <Servo.h> 

// Open audio serial communication on pin A0 at 20 bits per seconds
// with starting byte 138 and voltage threshold 30
AudioSerial audioserial(A0,20,138,30);

int previousValue = 0;
int count = 0;
void setup(){
 Serial.begin(9600);
 Serial.println("hello"); 
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

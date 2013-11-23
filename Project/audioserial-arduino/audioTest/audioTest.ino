#include <AudioSerial.h>

/* Audio Serial Test
 * Communicate between phone and microcontroller.
 * Partner android app, ServoControl at: https://github.com/AlvinYuan/cs294-84
 * 
 * Setup: Phone -> Arduino
 * Left AND Right Channel Stereo - 4k Ohm - GND
 * Left OR Right Channel Stereo - Op Amp + Input
 * Low Voltage (above phone bias) - Op Amp - Input (10k pot will help)
 * Op Amp Output - 100 Ohm - 10 pin
 * VCC and GND to Op Amp
 * GND to Audio Jack GND
 *
 * Setup: Arduino -> Phone 
 * Voltage Divider (1/2 factor) between D3 and GND
 * Voltage Divider Output - Audio Jack Mic
 * Audio Jack Mic - 10k Ohm - GND
*/

#include <SoftwareSerial.h>

// For Debugging
boolean debug;
const int debugPin = 4;

// Packet
const char packetDelimiter = '\n';
const char maxPacketSize = 100;
char packet[maxPacketSize];

// AudioSerial
SoftwareSerial audioSerial(10, 11, true); // RX, TX, inverted logic (logical 1 = LOW voltage, logical 0 = HIGH voltage)
int baudrate = 300;

void setup(){
  // AudioSerial
  audioSerial.begin(baudrate);

  // For Debugging
  Serial.begin(57600);
  Serial.println("hello"); 
  pinMode(debugPin, INPUT_PULLUP);
}

void loop(){
  debug = digitalRead(debugPin) == LOW;
  if (!debug) {
    if (audioSerial.available()) {      
      int numBytesRead = audioSerial.readBytesUntil(packetDelimiter, packet, maxPacketSize);
      Serial.print(numBytesRead);
      Serial.println(packet);
      for (int i = 0; i < numBytesRead; i++) {
        packet[i] = 0;
      }
    }
    
    
  } else {
    // Do Debugging Code
  }
}

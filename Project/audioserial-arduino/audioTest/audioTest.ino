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
const int debugPin = A5;

// Packet
const char packetDelimiter = '\n';
const char maxPacketSize = 100;
char packetRX[maxPacketSize];

boolean TX_Packet_Ready = false;
boolean TX_Available = true;
long startSendTime;
char packetTX[maxPacketSize];

// AudioSerial
const int audioSerialRX = 10;
const int audioSerialTX = 11;
SoftwareSerial audioSerial(audioSerialRX, audioSerialTX, true); // inverted logic (logical 1 = LOW voltage, logical 0 = HIGH voltage)
const long baudrate = 300;
const long oneSecondInMicros = 1000000;
const long bitsPerByte = 10;
const long baudrateTX = 20;
const long microsPerBit = oneSecondInMicros / baudrateTX;

void setup(){
  // AudioSerial
  audioSerial.begin(baudrate);
  audioSerial.write('a'); // BUG: SoftwareSerial TX default voltage is not inverted until after a write.

  // For Debugging
  Serial.begin(57600);
  Serial.println("hello"); 
  pinMode(debugPin, INPUT_PULLUP);
  pinMode(A0, INPUT);
  String str = "HelloLonger";
  str.toCharArray(packetTX, maxPacketSize);
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  pinMode(5, OUTPUT);
}

void loop(){
  debug = digitalRead(debugPin) == LOW;
  if (!debug) {
    audioSerialRXLoop();
    audioSerialTXLoop();

  } else {
    // Do Debugging Code
    char packet[2];
    packet[0] = 'a';
    packet[1] = 0;
    preparePacketTX(packet);

//    Serial.println(analogRead(A0));
  }

//  long time = millis();
//  int testBaudRate = 20;
//  if (time % (2 * 1000 / testBaudRate) < (1000 / testBaudRate)) {
//    analogWrite(5, 255/2);
//  } else {
//    analogWrite(5, 0);
//  }

//  if (time % 5000 < 1000) {
//    digitalWrite(2, HIGH);
//  } else if (time % 5000 < 2000) {
//    if ((time / 100) % 2 == 0) {
//      digitalWrite(2, LOW);
//    } else {
//      digitalWrite(2, HIGH);
//    }
//  } else {
//    digitalWrite(2, LOW);
//  }
}

// AudioSerial RX code that runs in the main loop
void audioSerialRXLoop() {
  if (audioSerial.available()) {
    int numBytesRead = audioSerial.readBytesUntil(packetDelimiter, packetRX, maxPacketSize);

    // Do Actions With packetRX
    Serial.print(numBytesRead);
    Serial.println(packetRX);

    // Clear packet data
    memset(packetRX, 0, numBytesRead);
  }
}

// AudioSerial TX code that runs in the main loop
// Special Protocol: First send a one second start bit, followed by a stop bit, then send regularly.
// This is so the phone can pick up the start bit without having to poll as often.
// TODO: gracefully handle micros() overflow
// TODO: change this to be like a state machine with actions on transitions, not states
void audioSerialTXLoop() {
  if (!TX_Available) {
    // Currently sending a packet. Do packet sending code
    long currentTime = micros();
    long timeSinceStart = currentTime - startSendTime;
    // Determine the state of transmitting the packet based on time
    if (timeSinceStart < oneSecondInMicros) {
      // Send one second start bit
//      digitalWrite(audioSerialTX, HIGH);
      analogWrite(5, 255/2);
    } else if (timeSinceStart < oneSecondInMicros + microsPerBit * 1) {
      // Send stop bit
//      digitalWrite(audioSerialTX, LOW);
      analogWrite(5, 0);
    } else if (timeSinceStart < oneSecondInMicros + microsPerBit * (maxPacketSize * bitsPerByte + 1)) { // Can probably be reduced based on actual packet size rather than max
      if (TX_Packet_Ready) {
        TX_Packet_Ready = false;
        audioSerial.write(packetTX);
        Serial.println("Sending packetTX");

        // Debugging
        char c = packetTX[0];
          // Send start bit
        analogWrite(5, 255/2);
        delay(microsPerBit / 1000);
        
        for (int i = 0; i < 8; i++) {
          // Inverted logic
          if ((c >> i) & 1 == 1) {
            analogWrite(5, 255/2);
          } else {
            analogWrite(5, 0);
          }
          delay(microsPerBit / 1000);
        }
        TX_Available = true;
        Serial.println("TX available");
      }
    } else {
        TX_Available = true;
        Serial.println("TX available");
    }

  } else if (TX_Packet_Ready) {
    // TX is available and a packet is ready to be sent. Initialize packet sending state
    TX_Available = false;
    startSendTime = micros();
    Serial.println("Begin sending");
  }
}

// Prepare packetTX and TX state for sending data.
// Does nothing if TX is not available. Caller may want to check this itself.
void preparePacketTX(String str) {
  if (!TX_Available) {
    return;
  }
  
  // Copy string to packetTX
  str.toCharArray(packetTX, maxPacketSize); // Also copies null character

  // Append packet delimiter char
  int len = str.length();
  packetTX[len] = packetDelimiter; // overwrite null with packetDelimiter
  packetTX[len+1] = 0; // add null. may not be necessary?
 
  // Set TX state
  TX_Packet_Ready = true;
}


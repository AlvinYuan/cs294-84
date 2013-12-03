#include <AudioSerial.h>
#include "enums.h"

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
 */

#include <SoftwareSerial.h>

// For Debugging
const int debugPin = A5;

// Constants
const int MAX_PACKET_SIZE = 100;
const char PACKET_DELIMITER = '\n';
const long BITS_PER_BYTE = 10;
const long ONE_SECOND_IN_MICROS = 1000000;

// Packet Buffers
char packetRX[MAX_PACKET_SIZE];
char packetTX[MAX_PACKET_SIZE];

// AudioSerial
const int audioSerialRX = 10;
const int audioSerialTX = 3; // try to stick to pin 3 or 11 here. See comments for setPwmFrequency()
const int DO_NOT_USE_PIN = 11;
SoftwareSerial audioSerial(audioSerialRX, DO_NOT_USE_PIN, true); // inverted logic (logical 1 = LOW voltage, logical 0 = HIGH voltage)
const long baudrateRX = 300;
const long baudrateTX = 100;
const long microsPerBit = ONE_SECOND_IN_MICROS / baudrateTX;

const long LONG_START_BIT_MICROS = ONE_SECOND_IN_MICROS;
const long STOP1_BIT_MICROS = microsPerBit;

// TX State
TX_state stateTX;
int TX_TRANSMIT_packetIndex = 0;
long startSendTime;

void setup(){
  // For Debugging
  Serial.begin(57600);
  Serial.println("hello"); 
  pinMode(debugPin, INPUT_PULLUP);
  pinMode(A0, INPUT);

  // AudioSerial
  audioSerial.begin(baudrateRX);
  // The bug mentioned below does not matter if we are not using SofwareSerial TX
//  audioSerial.write('a'); // BUG: SoftwareSerial TX default voltage is not inverted until after a write.
  pinMode(audioSerialTX, OUTPUT);
  // Increase the PWM frequency of audioSerialTX
  setPwmFrequency(audioSerialTX, 8);
  updateStateTX(TX_AVAILABLE);
}

void loop(){
  boolean debug = digitalRead(debugPin) == LOW;
  if (!debug) {
    audioSerialRXLoop();
    audioSerialTXLoop();

  } else {
    // Do Debugging Code
    preparePacketTX("abcde123");

//    Serial.println(analogRead(A0));
  }
}

// AudioSerial RX code that runs in the main loop
void audioSerialRXLoop() {
  if (audioSerial.available()) {
    int numBytesRead = audioSerial.readBytesUntil(PACKET_DELIMITER, packetRX, MAX_PACKET_SIZE);

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
// Functionality in the form of a state machine.
// TODO: gracefully handle micros() overflow
void audioSerialTXLoop() {
  long currentTime = micros();
  long timeSinceStart = currentTime - startSendTime;

  // Current state behavior and next state function
  switch (stateTX) {
    // TX is currently available (not busy transmitting)
    case TX_AVAILABLE:
      // Next state function
      if (packetTX[0] != 0) {
        updateStateTX(TX_LONG_START);
      }
      break;

    // TX is currently sending the initial one second start bit, as per the special protocol
    case TX_LONG_START:
      // Next state function
      if (timeSinceStart > LONG_START_BIT_MICROS) {
        updateStateTX(TX_STOP1);
      }
      break;

    // TX is currently sending the stop bit that follows the one second start bit, as per the special protocol
    case TX_STOP1:
      // Next state function
      if (timeSinceStart > LONG_START_BIT_MICROS + STOP1_BIT_MICROS) {
        updateStateTX(TX_TRANSMIT);
      }
      break;

    // TX is currently sending packetTX, at bitIndex of c.
    case TX_TRANSMIT:
      char c = packetTX[TX_TRANSMIT_packetIndex];
      // Current state behavior
      int bitIndex = ((timeSinceStart - LONG_START_BIT_MICROS - STOP1_BIT_MICROS) / microsPerBit) % BITS_PER_BYTE;
      if (bitIndex == 0) {
        // Send start bit
        analogWrite(audioSerialTX, 255/2);
      } else if (bitIndex == 9) {
        // Send stop bit
        analogWrite(audioSerialTX, 0);
      } else {
          if ((c >> (bitIndex-1)) & 1 == 1) {
            analogWrite(audioSerialTX, 255/2);
          } else {
            analogWrite(audioSerialTX, 0);
          }
      }
 
      // Next state function
      if (timeSinceStart > LONG_START_BIT_MICROS + STOP1_BIT_MICROS + microsPerBit * BITS_PER_BYTE * (TX_TRANSMIT_packetIndex + 1)) {
        if (c == '\n' || TX_TRANSMIT_packetIndex >= MAX_PACKET_SIZE) {
          updateStateTX(TX_AVAILABLE);
        } else {
          updateStateTX(TX_TRANSMIT);
        }
      }
      break;
  }
}

// state update behavior
void updateStateTX(TX_state newState) {
  switch (newState) {
    case TX_AVAILABLE:
      Serial.println("TX available");
      TX_TRANSMIT_packetIndex = -1;
      packetTX[0] = 0;
      analogWrite(audioSerialTX, 0);
      break;
    case TX_LONG_START:
//      Serial.println("Begin sending");
      startSendTime = micros();
      // Send one second start bit
      analogWrite(audioSerialTX, 255/2);
      break;
    case TX_STOP1:
//      Serial.println("Sending packetTX");
      // Send stop bit
      analogWrite(audioSerialTX, 0);
      break;
    case TX_TRANSMIT:
      TX_TRANSMIT_packetIndex++;
      break;
  }

  stateTX = newState;
}

// Prepare packetTX for sending data.
// Does nothing if TX is not available. Caller may want to check this itself.
void preparePacketTX(String str) {
  if (stateTX != TX_AVAILABLE) {
    return;
  }
  
  // Copy string to packetTX
  str.toCharArray(packetTX, MAX_PACKET_SIZE); // Also copies null character

  // Append packet delimiter char
  int len = str.length();
  packetTX[len] = PACKET_DELIMITER; // overwrite null with PACKET_DELIMITER
  packetTX[len+1] = 0; // add null. may not be necessary?
}

// http://playground.arduino.cc/Code/PwmFrequency
// Careful with this function. Can mess up timing.
// The above link includes a link to a forum post: http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1235060559/0#4
// In the post, it mentions that timer2 for pins 3 and 11 seem to be more independent (fewer timing side effects).
void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x7; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}

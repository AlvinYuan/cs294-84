/* Device Controller
 * Code to control the EPR device.
 * Tasks involve:
 * Bidirectional communication with phone app through audio jack.
 * Sensing button, switch. Controlling LEDs
 * Communication through Nordic.
 */

#include "enums.h"
// For AudioSerialRX
#include <SoftwareSerial.h>
// For Nordic Radio
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"


// Constants
const int MAX_PACKET_SIZE = 50;
const char PACKET_DELIMITER = '\n';
const long BITS_PER_BYTE = 10;
const long ONE_SECOND_IN_MICROS = 1000000;
const uint64_t RADIO_PIPE = 0xF0F0F0F0E1LL; // Radio pipe address for nordics to communicate.
const int AUDIO_SERIAL_TX_PWM_HIGH = 255/4;

// Pins
// Commented pins pertain to actual device (pro mini) when it differs from breadboard (boarduino).
const int audioSerialTX = 3;
const int audioSerialRX = 4;
const int piezoSpeaker = 6;
const int radioCS = 8;
const int radioCE = 9;
const int SPI_SS = 10; // not used by device, but the pin should not be used for other activity.
const int SPI_MOSI = 11;
const int SPI_MISO = 12;
const int SPI_SCK = 13;
const int sosNearbyLED = A0;
const int severeDangerLED = A1;
const int dangerNearbyLED = A2;
const int sosSwitch = A3;
const int DO_NOT_USE_PIN = A4; // For SoftwareSerialTX, which is not used.
const int sosOnLED = 5; //A6;
const int dangerButton = 7; //A7;

/*
 * Pins Additional Info
 * PWM Frequency
 * It is somewhat important that audioSerialTX is pin 3, due to setPwmFrequency having side effects with timing.
 * http://playground.arduino.cc/Code/PwmFrequency
 * http://forum.arduino.cc/index.php/topic,16612.0.html#4
 *
 * Nordic Hookup
 * http://maniacbug.wordpress.com/2011/03/19/2-4ghz-rf-radio-transceivers-and-library-8/
 * http://www.ebay.com/itm/2-4G-Wireless-nRF24L01-Module-/271021015267?pt=LH_DefaultDomain_0&hash=item3f1a1c80e3
 * SPI
 * It is also important that the SPI bus pins (MOSI, MISO, SCK, SS) are devoted for SPI activity.
 * The SPI pin constants above are not used by this code, but they are used by the device.
 * http://arduino.cc/en/Main/ArduinoBoardProMini
 * http://arduino.cc/en/Reference/SPI
 *
 * The remaining pins can be freely assigned.
 */

// Packet Buffers
char readAudioSerialPacket[MAX_PACKET_SIZE];
int readAudioSerialPacketSize;
char readRadioPacket[MAX_PACKET_SIZE];
int readRadioPacketSize;
char sendingAudioSerialPacket[MAX_PACKET_SIZE];
int sendingAudioSerialPacketSize;

// AudioSerial
SoftwareSerial audioSerial(audioSerialRX, DO_NOT_USE_PIN, true); // inverted logic (logical 1 = LOW voltage, logical 0 = HIGH voltage)
const long baudrateRX = 300;
const long baudrateTX = 200;

const long microsPerBit = ONE_SECOND_IN_MICROS / baudrateTX;
const long LONG_START_BIT_MICROS = ONE_SECOND_IN_MICROS;
const long STOP1_BIT_MICROS = microsPerBit;

// AudioSerial TX State
AudioSerialTX_State audioSerialStateTX;
int TX_TRANSMIT_packetIndex;
long sendAudioSerialPacketStartTime;

// Nordic Radio
RF24 radio(radioCE, radioCS);

// Danger Button
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers
int buttonState;            // the current reading from the input pin
int lastButtonState = HIGH; // the previous reading from the input pin

// SOS Switch
long lastSosSendTime = 0;
const long sosSendPeriod = 5000;

// LEDs
long lastSosReceivedTime = 0;
long lastSevereDangerReceivedTime = 0;
long lastDangerReceivedTime = 0;
int sosOnLEDState = LOW;

// Prepare communication channels and initial state
void setup(){
  // For Debugging
  Serial.begin(57600);
  Serial.println("hello"); 

  // AudioSerial
  audioSerial.begin(baudrateRX);
  pinMode(audioSerialTX, OUTPUT);
  setPwmFrequency(audioSerialTX, 8);
  updateStateTX(TX_AVAILABLE);
  
  // Nordic radio
  radio.begin();
  radio.setRetries(15,15); // optionally, increase the delay between retries & # of retries
  radio.openReadingPipe(0,RADIO_PIPE); // Open pipes initially for reading
  radio.startListening(); // Start listening

  // LEDs
  pinMode(sosNearbyLED, OUTPUT);
  pinMode(severeDangerLED, OUTPUT);
  pinMode(dangerNearbyLED, OUTPUT);
  pinMode(sosOnLED, OUTPUT);

  // Sensors
  pinMode(sosSwitch, INPUT_PULLUP);
  pinMode(dangerButton, INPUT_PULLUP);
}

void loop(){
  if (readRadio()) {
    // TODO: buffer radio messagess
    Serial.print(readRadioPacket);
    Serial.print(readRadioPacketSize);
    sendAudioSerialPacket(readRadioPacket, readRadioPacketSize);
    // TODO: parse packet and do special behavior

    // Clear packet data
    memset(readRadioPacket, 0, readRadioPacketSize);
  }
  if (readAudioSerial()) {
    // Do Actions With packetRX
    if (sendRadioPacket(readAudioSerialPacket, readAudioSerialPacketSize)) {
      Serial.println("sent");
    }
    Serial.println(readAudioSerialPacket);

    // Clear packet data
    memset(readAudioSerialPacket, 0, readAudioSerialPacketSize);
  }

  audioSerialTXLoop();

  dangerButtonLoop();

  sosSwitchLoop();

  // TESTING ONLY
  long currentTime = millis();
  int severeDangerLEDState = currentTime - lastDangerReceivedTime > 5000 ? LOW : HIGH;
  int dangerNearbyLEDState = currentTime - lastDangerReceivedTime > 5000 ? LOW : HIGH;
  int sosNearbyLEDState = currentTime - lastSosReceivedTime > 5000 ? LOW : HIGH;
  if (currentTime - lastDangerReceivedTime > 10000) {
    lastSevereDangerReceivedTime = currentTime;
    lastDangerReceivedTime = currentTime;
    lastSosReceivedTime = currentTime;
  }
  
  // Update LEDs
  digitalWrite(sosNearbyLED, sosNearbyLEDState);
  digitalWrite(severeDangerLED, severeDangerLEDState);
  digitalWrite(dangerNearbyLED, dangerNearbyLEDState);
  digitalWrite(sosOnLED, sosOnLEDState);
}

/********************
-- audioSerialTXLoop

audioSerial TX code that should run in the main loop.
Note that this is not interrupt driven, so it will have difficulty supporting high baud rates.
The main loop needs to be fast in order for this to work (definitely avoid calling delay).

Special audioSerial TX Protocol:
First send a one second start bit, followed by a stop bit, then send regularly.
This is so the phone can pick up the start bit without having to poll as often.

XXX: Behavior has not been checked for when micros() overflows.
********************/
void audioSerialTXLoop() {
  long currentTime = micros();
  long timeSinceStart = currentTime - sendAudioSerialPacketStartTime;

  // Current state behavior and next state function
  switch (audioSerialStateTX) {
    // TX is currently available (not busy transmitting)
    case TX_AVAILABLE:
      // Next state function
      if (sendingAudioSerialPacketSize != 0) {
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

    // TX is currently sending sendAudioSerialPacket, at bitIndex of c.
    case TX_TRANSMIT:
      char c = sendingAudioSerialPacket[TX_TRANSMIT_packetIndex];
      // Current state behavior
      int bitIndex = ((timeSinceStart - LONG_START_BIT_MICROS - STOP1_BIT_MICROS) / microsPerBit) % BITS_PER_BYTE;
      if (bitIndex == 0) {
        // Send start bit
        analogWrite(audioSerialTX, AUDIO_SERIAL_TX_PWM_HIGH);
      } else if (bitIndex == 9) {
        // Send stop bit
        analogWrite(audioSerialTX, 0);
      } else {
        int bitValue = ((c >> (bitIndex-1)) & 1) == 1 ? AUDIO_SERIAL_TX_PWM_HIGH : 0;
        analogWrite(audioSerialTX, bitValue);
      }
 
      // Next state function
      if (timeSinceStart > LONG_START_BIT_MICROS + STOP1_BIT_MICROS + microsPerBit * BITS_PER_BYTE * (TX_TRANSMIT_packetIndex + 1)) {
        if (TX_TRANSMIT_packetIndex >= sendingAudioSerialPacketSize) {
          updateStateTX(TX_AVAILABLE);
        } else {
          TX_TRANSMIT_packetIndex++;
        }
      }
      break;
  }
}

/********************
-- readAudioSerial

Returns true if we received a packet on the audioSerial connection.
If we did, modifies readAudioSerialPacket and readAudioSerialPacketSize with the read data.
readAudioSerialPacket should be well-formed, but that is up to the sender.
********************/
boolean readAudioSerial() {
  boolean received = false;
  if (audioSerial.available()) {
    readAudioSerialPacketSize = audioSerial.readBytesUntil(PACKET_DELIMITER, readAudioSerialPacket, MAX_PACKET_SIZE);
    received = true;
  }
  return received;
}

/********************
-- readRadio

Returns true if we received a packet on the radio.
If we did, modifies readRadioPacket and readRadioPacketSize with the read data.
readRadioPacket should be well-formed, but that is up to the sender.
TODO: handle case if packet spans more than one radio packet.
********************/
boolean readRadio() {
  boolean received = false;
  if (radio.available()) {
    readRadioPacketSize = radio.getPayloadSize();
    received = radio.read(readRadioPacket, readRadioPacketSize);
    // Find PACKET_DELIMITER
    for (int i = 0; i < readRadioPacketSize; i++) {
      if (readRadioPacket[i] == PACKET_DELIMITER) {
        readRadioPacketSize = i + 1;
        break;
      }
    }
  }
  return received;
}

/********************
-- sendAudioSerialPacket

Prepares sendingAudioSerialPacket and audioSerialTX_State for sending.
Packet should already be well-formed
Does nothing if audioSerialTX_State is not TX_AVAILABLE. Caller may want to check this as well.
********************/
void sendAudioSerialPacket(char packet[], int length) {
  if (audioSerialStateTX != TX_AVAILABLE) {
    return;
  }
  
  // Copy packet to sendingAudioSerialPacket
  // TODO: use faster method. memcpy?
  for (int i = 0; i < length; i++) {
    sendingAudioSerialPacket[i] = packet[i];
  }
  sendingAudioSerialPacketSize = length;
}

/********************
-- sendRadioPacket

Broadcasts a packet via the radio.
Packet should already be well-formed.
Returns true if the packet was successfully delivered (someone received it and ACKed).
********************/
boolean sendRadioPacket(char packet[], int length) {
  radio.stopListening();
  radio.openWritingPipe(RADIO_PIPE);

  boolean delivered = radio.write(packet, length);
  radio.openReadingPipe(0, RADIO_PIPE);
  radio.startListening();
  return delivered;
}

/******************** 
-- updateStateTX

State update function for audioSerial TX.
********************/
void updateStateTX(AudioSerialTX_State newState) {
  switch (newState) {
    case TX_AVAILABLE:
      TX_TRANSMIT_packetIndex = 0;
      sendingAudioSerialPacketSize = 0;
      analogWrite(audioSerialTX, 0);
      break;
    case TX_LONG_START:
      sendAudioSerialPacketStartTime = micros();
      // Send one second start bit
      analogWrite(audioSerialTX, AUDIO_SERIAL_TX_PWM_HIGH);
      break;
    case TX_STOP1:
      // Send stop bit
      analogWrite(audioSerialTX, 0);
      break;
    case TX_TRANSMIT:
      break;
  }

  audioSerialStateTX = newState;
}

/********************
-- dangerButtonLoop

Code to check whether the danger button has been pressed.
This should run in the main loop.
Based on Debounce example sketch (I think?)
********************/
void dangerButtonLoop() {
  // read the state of the button
  int reading = digitalRead(dangerButton);

  // check to see if you just pressed the button 
  // (i.e. the input went from LOW to HIGH),  and you've waited 
  // long enough since the last press to ignore any noise:  

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // Button Pressed
      if (buttonState == LOW) {
        dangerButtonPressed();
      }
    }
  }

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState = reading;
}

/********************
-- dangerButtonPressed

Broadcast a danger message via radio.
TODO: update message sent once protocol is fleshed out.
********************/
void dangerButtonPressed() {
  Serial.println("Danger button pressed");
  char packet[] = "|D|\n";
  sendRadioPacket(packet,5);
}

/********************
-- sosSwitchLoop

Code to check whether the SOS switch is on and periodically send SOS messages.
This should run in the main loop.
TODO: update message sent once protocol is fleshed out.
********************/
void sosSwitchLoop() {
  int reading = digitalRead(sosSwitch);
  sosOnLEDState = reading == HIGH ? LOW : HIGH;

  long currentTime = millis();
  if (currentTime - lastSosSendTime > sosSendPeriod) {

    if (reading == LOW) {
      Serial.println("SOS sending");
      char message[] = "|S|\n";
      sendRadioPacket(message, 5);
      lastSosSendTime = currentTime;
    }
  }
}

// From http://playground.arduino.cc/Code/PwmFrequency
/**
 * Divides a given PWM pin frequency by a divisor.
 * 
 * The resulting frequency is equal to the base frequency divided by
 * the given divisor:
 *   - Base frequencies:
 *      o The base frequency for pins 3, 9, 10, and 11 is 31250 Hz.
 *      o The base frequency for pins 5 and 6 is 62500 Hz.
 *   - Divisors:
 *      o The divisors available on pins 5, 6, 9 and 10 are: 1, 8, 64,
 *        256, and 1024.
 *      o The divisors available on pins 3 and 11 are: 1, 8, 32, 64,
 *        128, 256, and 1024.
 * 
 * PWM frequencies are tied together in pairs of pins. If one in a
 * pair is changed, the other is also changed to match:
 *   - Pins 5 and 6 are paired on timer0
 *   - Pins 9 and 10 are paired on timer1
 *   - Pins 3 and 11 are paired on timer2
 * 
 * Note that this function will have side effects on anything else
 * that uses timers:
 *   - Changes on pins 3, 5, 6, or 11 may cause the delay() and
 *     millis() functions to stop working. Other timing-related
 *     functions may also be affected.
 *   - Changes on pins 9 or 10 will cause the Servo library to function
 *     incorrectly.
 * 
 * Thanks to macegr of the Arduino forums for his documentation of the
 * PWM frequency divisors. His post can be viewed at:
 *   http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1235060559/0#4
 */
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


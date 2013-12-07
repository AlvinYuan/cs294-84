/* Device Controller
 * Code to control the EPR device.
 * Tasks involve:
 * Bidirectional communication with phone app through audio jack.
 * Sensing button, switch. Controlling LEDs
 * Communication through Nordic.
 */

#include "enums.h"
// For audioSerialRX
#include <SoftwareSerial.h>
// For Nordic
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"


// Constants
const int MAX_PACKET_SIZE = 50;
const char PACKET_DELIMITER = '\n';
const long BITS_PER_BYTE = 10;
const long ONE_SECOND_IN_MICROS = 1000000;
const uint64_t RADIO_PIPE = 0xF0F0F0F0E1LL; // Radio pipe address for nordics to communicate.

// Pins. Double check pins for actual device (ex: is TX pin = 6 or 2?)
const int audioSerialRX = 4;
const int audioSerialTX = 3;
const int DO_NOT_USE_PIN = A4;
const int radioCE = 7;
const int radioCS = 6;
const int dangerButton = A5; //A7;
const int sosSwitch = A3;
const int dangerNearbyLED = A2;
const int sosOnLED = A1; //13;
const int sosNearbyLED = A0;
const int piezoSpeaker = 5;
// Additional radio pins specified here (MOSI, MISO, SCK, VCC, GND):
// http://maniacbug.wordpress.com/2011/03/19/2-4ghz-rf-radio-transceivers-and-library-8/
// http://www.ebay.com/itm/2-4G-Wireless-nRF24L01-Module-/271021015267?pt=LH_DefaultDomain_0&hash=item3f1a1c80e3

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
const long baudrateTX = 100;

const long microsPerBit = ONE_SECOND_IN_MICROS / baudrateTX;
const long LONG_START_BIT_MICROS = ONE_SECOND_IN_MICROS;
const long STOP1_BIT_MICROS = microsPerBit;

// TX State
AudioSerialTX_State audioSerialStateTX;
int TX_TRANSMIT_packetIndex;
long sendAudioSerialPacketStartTime;

// Nordic Radio
// Set up nRF24L01 radio on SPI bus plus pins radioCE and radioCS
RF24 radio(radioCE, radioCS);

// Danger Button
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers
int buttonState;             // the current reading from the input pin
int lastButtonState = HIGH;   // the previous reading from the input pin

// SOS Switch
long lastSosSendTime = 0;
const long sosSendPeriod = 5000;

// LEDs
int sosOnLEDState = LOW;
long lastSosReceivedTime = 0;
long lastDangerReceivedTime = 0;

// Prepare communication channels and initial state
void setup(){
  // For Debugging
  Serial.begin(57600);
  Serial.println("hello"); 

  // AudioSerial
  audioSerial.begin(baudrateRX);
  pinMode(audioSerialTX, OUTPUT);
  updateStateTX(TX_AVAILABLE);
  
  // Setup and configure rf radio
  radio.begin();
  radio.setRetries(15,15); // optionally, increase the delay between retries & # of retries
  radio.openReadingPipe(0,RADIO_PIPE); // Open pipes initially for reading
  radio.startListening(); // Start listening

  // LEDs and Sensors
  pinMode(dangerButton, INPUT_PULLUP);
  pinMode(sosSwitch, INPUT_PULLUP);
  pinMode(sosOnLED, OUTPUT);
  pinMode(dangerNearbyLED, OUTPUT);
  pinMode(sosNearbyLED, OUTPUT);
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
  int dangerNearbyLEDState = currentTime - lastDangerReceivedTime > 5000 ? LOW : HIGH;
  int sosNearbyLEDState = currentTime - lastSosReceivedTime > 5000 ? LOW : HIGH;
  if (currentTime - lastDangerReceivedTime > 10000) {
    lastDangerReceivedTime = currentTime;
    lastSosReceivedTime = currentTime;
  }
  
  // Update LEDs
  digitalWrite(sosOnLED, sosOnLEDState);
  digitalWrite(dangerNearbyLED, dangerNearbyLEDState);
  digitalWrite(sosNearbyLED, sosNearbyLEDState);
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
      analogWrite(audioSerialTX, 255/2);
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


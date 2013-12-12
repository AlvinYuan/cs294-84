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
const int MAX_PACKET_SIZE = 32;
const char PACKET_DELIMITER = '\n';
const long BITS_PER_BYTE = 10;
const long ONE_SECOND_IN_MICROS = 1000000;
const uint64_t RADIO_PIPE = 0xF0F0F0F0E1LL; // Radio pipe address for nordics to communicate.
const int AUDIO_SERIAL_TX_PWM_HIGH = 255/4;
const long SOS_SEND_PERIOD = 5000;
const long LED_BLINK_PERIOD = 500;
const long LED_BLINK_DURATION = 2 * LED_BLINK_PERIOD;
const long LED_ON_DURATION = 5000; // includes blinking time
const long PIEZO_DURATION = 900;
const long DANGER_BUTTON_FLASH_DURATION = 200;

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
const int severeDangerLED = A0;
const int dangerNearbyLED = A1;
const int sosNearbyLED = A2;
const int sosSwitch = A3;
const int DO_NOT_USE_PIN = A4; // For SoftwareSerialTX, which is not used.
const int sosOnLED = 5;
const int dangerButton = 2;

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

// Ring Buffer for sending AudioSerial packets
const int RING_BUFFER_SIZE = 8;
char ringBuffer[RING_BUFFER_SIZE][MAX_PACKET_SIZE];
int ringBufferSizes[RING_BUFFER_SIZE];
int ringBufferNextSendIndex = 0;
int ringBufferNextQueueIndex = 0;

// Packet Protocol
// Should match Android Packet class
enum PacketType {
  TYPE_DANGER = 'D',
  TYPE_SOS = 'S',
  TYPE_NOT_SPECIFIED = '?'
};
const int packetTypeIndex = 1;

enum LevelOfDanger {
  DANGER_SMALL = '1',
  DANGER_MEDIUM = '2',
  DANGER_SEVERE = '3',
  DANGER_NOT_SPECIFIED = '?'
};
const int dangerLevelIndex = 3;


// AudioSerial
SoftwareSerial audioSerial(audioSerialRX, DO_NOT_USE_PIN, false);
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

// Danger Button
long lastDangerButtonSendTime = 0;

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

    // Do Actions with fresh readRadioPacket
      // Buffer radio messages
    int newRingBufferNextQueueIndex = (ringBufferNextQueueIndex + 1) % RING_BUFFER_SIZE;
    if (newRingBufferNextQueueIndex != ringBufferNextSendIndex) {
      for (int i = 0; i < readRadioPacketSize; i++) {
        ringBuffer[ringBufferNextQueueIndex][i] = readRadioPacket[i];
      }
      ringBufferSizes[ringBufferNextQueueIndex] = readRadioPacketSize;
      ringBufferNextQueueIndex = newRingBufferNextQueueIndex; 
    }

      // Parse packet
    long currentTime = millis();
    if (readRadioPacketSize > packetTypeIndex) {
      if (readRadioPacket[packetTypeIndex] == TYPE_SOS) {
        lastSosReceivedTime = currentTime;
      } else if (readRadioPacket[packetTypeIndex] == TYPE_DANGER) {
        lastDangerReceivedTime = currentTime;
        if (readRadioPacketSize > dangerLevelIndex && readRadioPacket[dangerLevelIndex] == DANGER_SEVERE) {
          lastSevereDangerReceivedTime = currentTime;
        }
      }
    }

    // Clear packet data
    memset(readRadioPacket, 0, readRadioPacketSize);
  }

  // Send AudioSerial packets if possible
  if (audioSerialStateTX == TX_AVAILABLE && ringBufferNextSendIndex != ringBufferNextQueueIndex) {
    sendAudioSerialPacket(ringBuffer[ringBufferNextSendIndex], ringBufferSizes[ringBufferNextSendIndex]);
    ringBufferNextSendIndex = (ringBufferNextSendIndex + 1) % RING_BUFFER_SIZE;
  }

  if (readAudioSerial()) {
    // TODO: detect phone connect (another packet type, sent by phone upon connecting)

    // Do Actions with fresh readAudioSerialPacket
      // Parse packet
    long currentTime = millis();
    if (readAudioSerialPacketSize > packetTypeIndex) {
      if (readAudioSerialPacket[packetTypeIndex] == TYPE_SOS) {
        if (digitalRead(sosSwitch) == HIGH) {
          // Switch is on.
          // Send read packet and update last SOS send time.
          // Expects that phone will send SOS messages periodically
          lastSosSendTime = currentTime + SOS_SEND_PERIOD;
          sendRadioPacket(readAudioSerialPacket, readAudioSerialPacketSize);
        }
      } else if (readAudioSerialPacket[packetTypeIndex] == TYPE_DANGER) {
        // Broadcast danger packet out
        sendRadioPacket(readAudioSerialPacket, readAudioSerialPacketSize);
      }
    }

    // Clear packet data
    memset(readAudioSerialPacket, 0, readAudioSerialPacketSize);
  }

  audioSerialTXLoop();

  dangerButtonLoop();

  sosSwitchLoop();

  // Piezo
  long currentTime = millis();
  if (   currentTime - lastSosReceivedTime < PIEZO_DURATION / 3
      || currentTime - lastDangerReceivedTime < PIEZO_DURATION / 3) {
    analogWrite(piezoSpeaker, 255/2);
  } else if (   currentTime - lastSosReceivedTime < PIEZO_DURATION * 2 / 3
             || currentTime - lastDangerReceivedTime < PIEZO_DURATION * 2/ 3) {
    analogWrite(piezoSpeaker, 0);
  } else if (   currentTime - lastSosReceivedTime < PIEZO_DURATION
             || currentTime - lastDangerReceivedTime < PIEZO_DURATION) {
    analogWrite(piezoSpeaker, 255/2);
  } else {
    analogWrite(piezoSpeaker, 0);
  }

  updateLEDsLoop();
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
    // TX has finished, but must wait till phone is ready before sending again.
    case TX_FINISHED:
      // Next state function
      if (timeSinceStart > LONG_START_BIT_MICROS + STOP1_BIT_MICROS + microsPerBit * BITS_PER_BYTE * MAX_PACKET_SIZE + ONE_SECOND_IN_MICROS * 3 / 2) {
        // 1.5 extra seconds of wait to let things stabilize
        updateStateTX(TX_AVAILABLE);
      }
      break;

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
          updateStateTX(TX_FINISHED);
        } else {
          TX_TRANSMIT_packetIndex++;
        }
      }
      break;
  }
}

/******************** 
-- blinkThenHoldLEDState

Returns the state of the LED (LOW/HIGH) based on currentTime and initialTime.
Causes the LED to blink then be held on for some duration.
********************/
int blinkThenHoldLEDState(long currentTime, long initialTime) {
  long difference = currentTime - initialTime;
  if (difference < LED_BLINK_DURATION) {
    if ((difference % LED_BLINK_PERIOD) < (LED_BLINK_PERIOD / 2)) {
      return HIGH;
    } else {
      return LOW;
    }
  } else if (difference < LED_ON_DURATION) {
    return HIGH;
  } else {
    return LOW;
  }
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
  lastDangerButtonSendTime = millis();
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
    readAudioSerialPacket[readAudioSerialPacketSize] = PACKET_DELIMITER;
    readAudioSerialPacketSize++;
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
-- setPwmFrequency

From http://playground.arduino.cc/Code/PwmFrequency
Link includes description.
********************/
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

/********************
-- sosSwitchLoop

Code to check whether the SOS switch is on and periodically send SOS messages.
This should run in the main loop.
********************/
void sosSwitchLoop() {
  int reading = digitalRead(sosSwitch);

  long currentTime = millis();
  if (currentTime - lastSosSendTime > SOS_SEND_PERIOD) {

    if (reading == HIGH) {
      Serial.println("SOS sending");
      char message[] = "|S|\n";
      sendRadioPacket(message, 5);
      lastSosSendTime = currentTime;
    }
  }
}

/******************** 
-- updateLEDsLoop

Code to update the state of the LEDs.
This should run in the main loop.
********************/
void updateLEDsLoop() {
  long currentTime = millis();
  int sosNearbyLEDState = blinkThenHoldLEDState(currentTime, lastSosReceivedTime);
  int severeDangerLEDState = blinkThenHoldLEDState(currentTime, lastSevereDangerReceivedTime);
  int dangerNearbyLEDState = blinkThenHoldLEDState(currentTime, lastDangerReceivedTime);
  // sosOnLEDState set in sosSwitchLoop(), but will flash once based on lastDangerButtonSendTime;
  if (currentTime - lastDangerButtonSendTime < DANGER_BUTTON_FLASH_DURATION) {
    sosOnLEDState = HIGH;
  } else if (currentTime - lastDangerButtonSendTime < DANGER_BUTTON_FLASH_DURATION * 2) {
    sosOnLEDState = LOW;
  } else if (currentTime - lastDangerButtonSendTime < DANGER_BUTTON_FLASH_DURATION * 3) {
    sosOnLEDState = HIGH;
  } else if (currentTime - lastDangerButtonSendTime < DANGER_BUTTON_FLASH_DURATION * 4) {
    sosOnLEDState = LOW;
  } else {
    int reading = digitalRead(sosSwitch);
    sosOnLEDState = reading;
  }

  // Update LEDs
  digitalWrite(sosNearbyLED, sosNearbyLEDState);
  digitalWrite(severeDangerLED, severeDangerLEDState);
  digitalWrite(dangerNearbyLED, dangerNearbyLEDState);
  digitalWrite(sosOnLED, sosOnLEDState);
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
    case TX_FINISHED:
      analogWrite(audioSerialTX, 0);
      break;
  }

  audioSerialStateTX = newState;
}



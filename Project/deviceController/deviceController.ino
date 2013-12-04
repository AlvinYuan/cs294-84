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

// Pins
const int audioSerialRX = 10;
const int audioSerialTX = 3; // try to stick to pin 3 or 11 here. See comments for setPwmFrequency()
const int DO_NOT_USE_PIN = 11;
const int radioCE = 9;
const int radioCS = 10;
const int dangerLED = 4;
const int sosLED = 5;
const int thirdLED = 6;
const int dangerButton = 7;
const int sosSwitch = 8;

// Packet Buffers
char packetRX[MAX_PACKET_SIZE];
char packetTX[MAX_PACKET_SIZE];

// AudioSerial
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

// Nordic Radio
// Set up nRF24L01 radio on SPI bus plus pins radioCE and radioCS
RF24 radio(radioCE, radioCS);
char readMessage[MAX_PACKET_SIZE];
int readMessageSize = 0;

// Danger Button
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin

// SOS Switch
long lastSosSendTime = 0;
const long sosSendPeriod = 5000;

// LEDs
int dangerLEDState = LOW;
int sosLEDState = LOW;
int thirdLEDState = LOW;

// Prepare communication channels and initial state
void setup(){
  // For Debugging
  Serial.begin(57600);
  Serial.println("hello"); 

  // AudioSerial
  audioSerial.begin(baudrateRX);
  pinMode(audioSerialTX, OUTPUT);
  setPwmFrequency(audioSerialTX, 8); // Increase the PWM frequency of audioSerialTX
  updateStateTX(TX_AVAILABLE);
  
  // Setup and configure rf radio
  radio.begin();
  radio.setRetries(15,15); // optionally, increase the delay between retries & # of retries
  radio.openReadingPipe(0,RADIO_PIPE); // Open pipes initially for reading
  radio.startListening(); // Start listening

  // LEDs and Sensors
  pinMode(dangerLED, OUTPUT);
  digitalWrite(dangerLED, LOW);
  pinMode(sosLED, OUTPUT);
  digitalWrite(sosLED, LOW);
  pinMode(thirdLED, OUTPUT);
  digitalWrite(thirdLED, LOW);
  pinMode(dangerButton, INPUT_PULLUP);
  pinMode(sosSwitch, INPUT_PULLUP);
}

void loop(){
  if (checkForRadioMessageLoop()) {
    // TODO: buffer radio messages
    preparePacketTX(readMessage);
    // TODO: parse packet and do special behavior

    if (readMessage[1] == 'D') {
      // For Testing
      dangerLEDState = dangerLEDState == HIGH ? LOW : HIGH;
    } else {
      // For Testing
      thirdLEDState = thirdLEDState == HIGH ? LOW : HIGH;
    }
  }
  audioSerialRXLoop();
  audioSerialTXLoop();
  buttonLoop();
  digitalWrite(dangerLED, dangerLEDState);
  digitalWrite(sosLED, sosLEDState);
  digitalWrite(thirdLED, thirdLEDState);
}

// AudioSerial RX code that runs in the main loop
void audioSerialRXLoop() {
  if (audioSerial.available()) {
    int numBytesRead = audioSerial.readBytesUntil(PACKET_DELIMITER, packetRX, MAX_PACKET_SIZE);

    // Do Actions With packetRX
    broadcastMessage(packetRX, numBytesRead);
    // For Testing
    thirdLEDState = thirdLEDState == HIGH ? LOW : HIGH;

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
      TX_TRANSMIT_packetIndex = -1;
      packetTX[0] = 0;
      analogWrite(audioSerialTX, 0);
      break;
    case TX_LONG_START:
      startSendTime = micros();
      // Send one second start bit
      analogWrite(audioSerialTX, 255/2);
      break;
    case TX_STOP1:
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
  packetTX[len+1] = 0; //add null char
}

boolean broadcastMessage(char message[], int length) {
  radio.stopListening();
  radio.openWritingPipe(RADIO_PIPE);

  boolean delivered = radio.write(message, length);
  radio.openReadingPipe(0, RADIO_PIPE);
  radio.startListening();
  return delivered;
}

boolean checkForRadioMessageLoop() {
  boolean received = false;
  if (radio.available()) {
    readMessageSize = radio.getPayloadSize();
    received = radio.read(readMessage, readMessageSize);
    readMessage[readMessageSize] = 0; // TODO: check if necessary. Also check for double \n with preparePacketTX
  }
  return received;
}

void buttonLoop() {
    // read the state of the switch into a local variable:
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
        // TODO: message protocol
        char message[] = "|D|\n";
        broadcastMessage(message, 7);
      }
    }
  }

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState = reading;
}

void sosSwitchLoop() {
  long currentTime = millis();
  if (currentTime - lastSosSendTime > sosSendPeriod) {
    int reading = digitalRead(sosSwitch);

    // For Testing
    sosLEDState = reading;

    if (reading == LOW) {
      // TODO: custom message
      // TODO: message protocol
      char message[] = "|S|\n";
      broadcastMessage(message, 7);
      lastSosSendTime = currentTime;
    }
  }
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

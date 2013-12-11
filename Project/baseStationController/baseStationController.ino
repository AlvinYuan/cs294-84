/* Base Station Controller
 * Code to control the base station device.
 * Very rudimentary right now.
 * Tasks involve:
 * Communication through Nordic.
 * Reading packet data to determine SOS or Danger message.
 * Periodically broadcasting out the last SOS and Danger message received.
 */

// For Nordic Radio
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

// Constants
const int MAX_PACKET_SIZE = 50;
const char PACKET_DELIMITER = '\n';
const long BITS_PER_BYTE = 10;
const uint64_t RADIO_PIPE = 0xF0F0F0F0E1LL; // Radio pipe address for nordics to communicate.
const long RESEND_PERIOD = 10000;

// Pins
// Commented pins pertain to actual device (pro mini) when it differs from breadboard (boarduino).
const int radioCS = 8;
const int radioCE = 9;
const int SPI_SS = 10; // not used by device, but the pin should not be used for other activity.
const int SPI_MOSI = 11;
const int SPI_MISO = 12;
const int SPI_SCK = 13;

/*
 * Pins Additional Info
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
char readRadioPacket[MAX_PACKET_SIZE];
int readRadioPacketSize;
char lastSOSPacket[MAX_PACKET_SIZE];
int lastSOSPacketSize;
char lastDangerPacket[MAX_PACKET_SIZE];
int lastDangerPacketSize;

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

// Nordic Radio
RF24 radio(radioCE, radioCS);

// Packet Timing
long lastSOSSendTime = 0;
long lastDangerSendTime = 0;

// Prepare communication channels and initial state
void setup(){
  // For Debugging
  Serial.begin(57600);
  Serial.println("hello base station"); 
  
  // Nordic radio
  radio.begin();
  radio.setRetries(15,15); // optionally, increase the delay between retries & # of retries
  radio.openReadingPipe(0,RADIO_PIPE); // Open pipes initially for reading
  radio.startListening(); // Start listening
}

void loop(){
  long currentTime = millis();
  if (readRadio()) {
    // Do Actions with fresh readRadioPacket
    Serial.println(readRadioPacket);

      // Parse packet
    long currentTime = millis();
    if (readRadioPacketSize > packetTypeIndex) {
      if (readRadioPacket[packetTypeIndex] == TYPE_SOS) {
        for (int i = 0; i < readRadioPacketSize; i++) {
          lastSOSPacket[i] = readRadioPacket[i];
        }
        lastSOSPacketSize = readRadioPacketSize;
        // Don't resend immediately
        lastSOSSendTime = currentTime;
      } else if (readRadioPacket[packetTypeIndex] == TYPE_DANGER) {
        for (int i = 0; i < readRadioPacketSize; i++) {
          lastDangerPacket[i] = readRadioPacket[i];
        }
        lastDangerPacketSize = readRadioPacketSize;
        // Don't resend immediately
        lastDangerSendTime = currentTime;
      }
    }

    // Clear packet data
    memset(readRadioPacket, 0, readRadioPacketSize);
  }
  
  if (currentTime - lastSOSSendTime > RESEND_PERIOD && lastSOSPacketSize != 0) {
    Serial.println("Sending SOS Message");
    sendRadioPacket(lastSOSPacket, lastSOSPacketSize);
    lastSOSSendTime = currentTime;
  }
  if (currentTime - lastDangerSendTime > RESEND_PERIOD && lastDangerPacketSize != 0) {
    Serial.println("Sending Danger Message");
    sendRadioPacket(lastDangerPacket, lastDangerPacketSize);
    lastDangerSendTime = currentTime;
  }
  
  if (Serial.available()) {
    // Send Custom Message
    Serial.println("Sending Custom Message");
    char message[MAX_PACKET_SIZE];
    int bytesRead = Serial.readBytesUntil(PACKET_DELIMITER, message, MAX_PACKET_SIZE);
    message[bytesRead] = PACKET_DELIMITER;
    if (sendRadioPacket(message, bytesRead+1)) {
      Serial.println("Custom Message Sent");
    } else {
      Serial.println("Custom Message Dropped");
    }
  }

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


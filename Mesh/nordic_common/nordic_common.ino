/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

/**
 * Nordic Common
 *
 * This is a sketch that allows two nordics to communicate to each other
 * without each taking on a certain role (ping/pong).
 * Connect a button between ground and D2 and connect the nordic as
 * per http://maniacbug.wordpress.com/2011/11/02/getting-started-rf24/
 * Pressing the button should send "Danger" to the other nordic.
 * Both devices communicate on the same pipe in both directions.
 * Something like this can possibly work with multiple (3+) devices assuming
 * that transmitting packets is sparse, i.e. most devices most of the time
 * are just listening.
 * This has not been tested with 3+ devices yet.
 */
 
 /*
 Requires RF24 library to run,
 modified 11/7/13 Colin Ho, Rundong Tian, Alvin Yuan
 */
 
 
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10
RF24 radio(9,10);

// Button read
const int buttonPin = 2;

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin

//
// Topology
//

// Radio pipe address for nodes to communicate.
const uint64_t pipe = 0xF0F0F0F0E1LL;

void setup(void)
{
  // Set up send pin
  pinMode(buttonPin, INPUT_PULLUP);
  //
  // Print preamble
  //

  Serial.begin(57600);
  printf_begin();
  printf("\n\rRF24/examples/pingpair/\n\r");

  //
  // Setup and configure rf radio
  //

  radio.begin();

  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15,15);

  // optionally, reduce the payload size.  seems to
  // improve reliability
//  radio.setPayloadSize(8);

  // Open pipes initially for reading
    radio.openReadingPipe(0,pipe);

  //
  // Start listening
  //

  radio.startListening();

  //
  // Dump the configuration of the rf unit for debugging
  //
  radio.printDetails();
}

void loop(void)
{
  // read the state of the switch into a local variable:
  int reading = digitalRead(buttonPin);

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
        // Send Message
        Serial.println("Sending Danger Message");
        radio.stopListening();
        radio.openWritingPipe(pipe);
        char message[15] = "|D|?|2|custom\n";
        bool delivered = radio.write(message, 14);
        radio.openReadingPipe(0, pipe);
        radio.startListening();
        if (delivered) {
          Serial.println("Danger Message Sent");
        } else {
          Serial.println("Danger Message Dropped");
        }
      }
    }
  }

  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState = reading;

  // Radio Reading
  
  if (radio.available()) {
    char message[radio.getPayloadSize()];
    // Dump the payloads until we've gotten everything

    // Fetch the payload, and see if this was the last one.
    boolean received = radio.read(message, radio.getPayloadSize());
    if (received) {
      Serial.print("Received Message: ");
      Serial.println(message);
    } else {
      Serial.println("Receive failed");
    }
  }
}

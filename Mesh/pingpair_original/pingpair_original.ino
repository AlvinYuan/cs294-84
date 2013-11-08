/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

/**
 * Example RF Radio Ping Pair
 *
 * This is an example of how to use the RF24 class.  Write this sketch to two different nodes,
 * connect the role_pin to ground on one.  The ping node sends the current time to the pong node,
 * which responds by sending the value back.  The ping node can then see how long the whole cycle
 * took.
 */
 
 /*
 Requires RF24 library to run,
 modified 11/7/13 Colin Ho
 */
 

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include <Servo.h>
//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

RF24 radio(9,10);

// Add servo
Servo servo1;
int pos = 0; //store servo position

// sets the role of this unit in hardware.  Connect to GND to be the 'pong' receiver
// Leave open to be the 'ping' transmitter
const int role_pin = 7;

// Pin to read button
const int button_pin = 6;

// Pin for servo
const int servo_pin = 5;

//initialize state variable that stores the status of the state
unsigned long state = 100;  //default off
//
// Topology
//

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

//
// Role management
//
// Set up role.  This sketch uses the same software for all the nodes
// in this system.  Doing so greatly simplifies testing.  The hardware itself specifies
// which node it is.
//
// This is done through the role_pin
//

// The various roles supported by this sketch
typedef enum { role_ping_out = 1, role_pong_back } role_e;

// The debug-friendly names of those roles
const char* role_friendly_name[] = { "invalid", "Ping out", "Pong back"};

// The role of the current running sketch
role_e role;

void setup(void)
{
  //
  // Role
  //

  // set up the role pin
  pinMode(role_pin, INPUT);
  digitalWrite(role_pin,HIGH);
  pinMode(button_pin,INPUT);
  delay(20); // Just to get a solid reading on the role pin
  servo1.attach(servo_pin);  //attach servo
  servo1.write(90);
  
  // read the address pin, establish our role
  if ( ! digitalRead(role_pin) )
    role = role_ping_out;
  else
    role = role_pong_back;

  //
  // Print preamble
  //

  Serial.begin(57600);
  printf_begin();
  printf("\n\rRF24/examples/pingpair/\n\r");
  printf("ROLE: %s\n\r",role_friendly_name[role]);

  //
  // Setup and configure rf radio
  //

  radio.begin();

  // optionally, increase the delay between retries & # of retries
  radio.setRetries(15,15);

  // optionally, reduce the payload size.  seems to
  // improve reliability
  radio.setPayloadSize(8);

  //
  // Open pipes to other nodes for communication
  //

  // This simple sketch opens two pipes for these two nodes to communicate
  // back and forth.
  // Open 'our' pipe for writing
  // Open the 'other' pipe for reading, in position #1 (we can have up to 5 pipes open for reading)

  if ( role == role_ping_out )
  {
    radio.openWritingPipe(pipes[0]);
    radio.openReadingPipe(1,pipes[1]);
  }
  else
  {
    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1,pipes[0]);
  }

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
  //
  // Ping out role.  Repeatedly send the current time
  //
  
  //Check state of button
  if (digitalRead(button_pin) == HIGH)
  {
    //state is high  
    state = 111; 
  }
  else  //button is LOW
  {
    state = 100; 
  }
  
  if (role == role_ping_out)
  {
    // First, stop listening so we can talk.
    radio.stopListening();

    //send the current state
    printf("Now sending %lu...",state);
    bool ok = radio.write( &state, sizeof(unsigned long) );
    
    if (ok)
      printf("ok...");
    else
      printf("failed.\n\r");

    // Now, continue listening
    radio.startListening();

    // Wait here until we get a response, or timeout (250ms)
    unsigned long started_waiting_at = millis();
    bool timeout = false;
    while ( ! radio.available() && ! timeout )
      if (millis() - started_waiting_at > 200 )
        timeout = true;

    // Describe the results
    if ( timeout )
    {
      printf("Failed, response timed out.\n\r");
    }
    else
    {
      // Grab the response, compare, and send to debugging spew
      unsigned long got_state;
      radio.read( &got_state, sizeof(unsigned long) );

      // Spew it
      printf("Got response %lu",got_state);
    }

    // Try again 1s later
    delay(100);
  }

  //
  // Pong back role.  Receive each packet, dump it out, and send it back
  //

  if ( role == role_pong_back )
  {
    // if there is data ready
    if ( radio.available() )
    {
      // Dump the payloads until we've gotten everything
      unsigned long got_state;
      bool done = false;
      while (!done)
      {
        // Fetch the payload, and see if this was the last one.
        done = radio.read( &got_state, sizeof(unsigned long) );

        // Spew it
        printf("Got payload %lu...",got_state);
        
	// Delay just a little bit to let the other unit
	// make the transition to receiver
	delay(20);
      }
      state = got_state;
      
      // actuate servo based on state change
      if (state == 100)
      {
        pos = 150;
        servo1.write(pos);  
      }
      else if (state == 111)
      {
        pos = 40;
        servo1.write(pos);
      }
      else
      {
        servo1.write(pos);
      }
      
      
      // First, stop listening so we can talk
      radio.stopListening();

      // Send the final one back.
      radio.write( &got_state, sizeof(unsigned long) );
      printf("Sent response.\n\r");

      // Now, resume listening so we catch the next packets.
      radio.startListening();
    }
  }
}
// vim:cin:ai:sts=2 sw=2 ft=cpp

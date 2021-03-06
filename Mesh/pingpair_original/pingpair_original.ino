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
 modified 11/7/13 Colin Ho, Rundong Tian, Alvin Yuan
 */
 
 
#include <AudioSerial.h> 
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

// Open audio serial communication on pin A0 at 20 bits per seconds
// with starting byte 138 and voltage threshold 30
AudioSerial audioserial(A0,20,138,30);


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
int servo_angle = 100;
// Pin to send to micropohone buttton
const int mic_pin = 8;

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
  pinMode(button_pin,INPUT_PULLUP);
  pinMode(mic_pin, OUTPUT);
  digitalWrite(mic_pin, LOW);
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
  // Ping out role.  
  // Remote device HW5
  //
   if (role == role_ping_out)
  {
    boolean gotAState = false;
    unsigned long got_state = state;

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
      radio.read( &got_state, sizeof(unsigned long) );
      gotAState = true;
      // Spew it
      printf("Got response %lu",got_state);
    }

    boolean buttonPinValue = digitalRead(button_pin);
    switch(state) {
      case 100:
        if (buttonPinValue == HIGH) {
          state = 111;
        }
        break;
      case 111:
        if (gotAState && got_state == 222) {
          state = got_state;
        }
        break;
      case 222:
        // actuate servo based on state change
        servo1.write(servo_angle);
        servo_angle = servo_angle == 100 ? 0 : 100;
        break;
    }
    
    // Try again 1s later
    delay(100);
  }

  //
  // Pong back role.  Receive each packet, dump it out, and send it back
  // Connected device HW5
  //

  if ( role == role_pong_back )
  {
    printf("%d state\n", state);
    char receivebyte;
    switch (state) {
      case 100:
        // if there is data ready
        if ( radio.available() )
        {
          // Dump the payloads until we've gotten everything
          unsigned long got_state;

          // Fetch the payload, and see if this was the last one.
          while (!radio.read( &got_state, sizeof(unsigned long) ))
          {
    	    // Delay just a little bit to let the other unit
    	    // make the transition to receiver
    	    delay(20);
          }
          // Spew it
          printf("state is %lu...",got_state);

          if (got_state == 111) {
            state = got_state;
          }
        }
       break;
      case 111:
        digitalWrite(mic_pin, HIGH);
        //Check for input from phone
        audioserial.run(); // looks for start bit
        
        receivebyte=audioserial.read();
        if(receivebyte>-1){    
          if (receivebyte =='R'){
            state = 222;
            printf("Received R from Phone");
          }
        } 
       break;
     case 222:
        // First, stop listening so we can talk
        radio.stopListening();
    
        // Send the final one back.
        radio.write( &state, sizeof(unsigned long) );
        printf("Sent response.\n\r");
    
        // Now, resume listening so we catch the next packets.
        radio.startListening();
        break;
    }      
  }
}
// vim:cin:ai:sts=2 sw=2 ft=cpp

/*
  AudioSerial.h - Audio serial library for communication from Android smartphone to Arduino
  Created by Guillaume Dupre, Friday 26, 2013.
  Released into the public domain.
*/
#ifndef AudioSerial_h
#define AudioSerial_h

#include "Arduino.h"

class AudioSerial
{
  private:
    int _pin;
    int _bps;
    boolean _readingBitState;
    int _bitPower;
    int _receiveByte;
    int _byteBuffer;
    int _reads[8];
    int _readIndex;
    long _lastReadTime;
    	
  public:
    AudioSerial(int pin,int bps);
    void run();
    int read();
    int getDelay();

};

#endif

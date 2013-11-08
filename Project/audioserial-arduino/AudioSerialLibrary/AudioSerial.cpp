/*
  AudioSerial.cpp - Audio serial library for communication from Android smartphone to Arduino
  Created by Guillaume Dupre, Friday 26, 2013.
  Released into the public domain.
*/

#include "Arduino.h"
#include "AudioSerial.h"

AudioSerial::AudioSerial(int pin,int bps,int startingByte,int threshold)
{
  _pin = pin;
  _bps = bps;
  _startingByte=startingByte;
  _threshold=threshold;
  _readingBitState=false;
  _readingByteState=false;
  _bitPower=1;
  _receiveByte=-1;
  _readIndex=0;
  _lastReadTime = 0;
}

void AudioSerial::run(){
  long currentTime = millis();
  if ( currentTime < _lastReadTime + 1000*2 / _bps) { // baudrate essentially halved to accommodate for new protocol of bit={bit,0}.
    return;
  }

  
  int receiveAnalog=analogRead(_pin);
  int receiveBit=0;
  // Compute bit from voltage
  if(receiveAnalog>_threshold){receiveBit=1;}
  // 
  if(!_readingBitState)
  {
 	 if(receiveBit==1){
     Serial.print("start bit found at ");
     Serial.println(currentTime);
     _lastReadTime = currentTime + 1000 / _bps / 2; // offset to try to start in the middle of the first bit.
  	_readingBitState=true;
 	 _bitPower=1;
 	 _byteBuffer=0;
     _readIndex=0;
	  }
  }else{
  Serial.print("read ");
  Serial.println(receiveBit);
  _lastReadTime = currentTime;
	_byteBuffer+=receiveBit*_bitPower;
        _bitPower=_bitPower*2;
        _reads[_readIndex++]=currentTime;
  	if(_bitPower==256){	
 		_readingBitState=false;
	
 		if(_readingByteState){
		_receiveByte=_byteBuffer;
 		}else if(_byteBuffer==_startingByte){
		_readingByteState=true;		
		}
	}
        
  }
  }

int AudioSerial::read(){
  int receiveByte=_receiveByte;
  _receiveByte=-1;
  if (receiveByte != -1) {  
      for (int i = 0; i < 8; i++) {  
        Serial.println(_reads[i]);
      }
  }
  return receiveByte;

  }

int AudioSerial::getDelay(){
 return 1000/_bps;
}



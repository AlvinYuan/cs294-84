/*
  AudioSerial.java - Audio serial library for communication from Android smartphone to Arduino
  Created by Guillaume Dupre, Friday 26, 2013.
  Released into the public domain.
*/

package com.audioserial.servocontrol;

import java.util.Random;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

public class AudioSerial {
	AudioTrack audiotrack;
	short[] low;
	short[] high;
	Random r;

	int bitlength;
	int startingbyte;
	// bps : bits per second, 20 recommended
	public AudioSerial(int bps,int startingbyte){
	    r = new Random();
		bitlength=44100/bps;
		this.startingbyte=startingbyte;
		audiotrack= new AudioTrack(AudioManager.STREAM_MUSIC, 44100, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT, 2000, AudioTrack.MODE_STREAM);
		// Initialization of low and high waveforms
		low=new short[bitlength];
		high=new short[bitlength];
		for(int i=0;i<bitlength;i++){
			low[i]=1;
			high[i]= Short.MAX_VALUE;
		}
		audiotrack.play();
		send(startingbyte);
	}

	// NOTE: protocol changed. Every bit is now the bit plus a zero bit.
	// This is because for some reason, sending consecutive 1's would not
	// always keep the voltage high. Huge cut to efficiency.
	// Sending one byte starting from the least significant bit
	public void send(int val){
		// Starting bit
		audiotrack.write(high, 0, bitlength);
		audiotrack.write(low, 0, bitlength);

		for(int i=0;i<8;i++){
			if(val%2==0){
				audiotrack.write(low, 0, bitlength);
				audiotrack.write(low, 0, bitlength);
			}else{
				audiotrack.write(high, 0, bitlength);
				audiotrack.write(low, 0, bitlength);
			}
			val=(val/2);
		}
		
		
		
// Static mode of AudioSerial. Not updated with new protocol mentioned above.
//        audiotrack= new AudioTrack(AudioManager.STREAM_MUSIC, 44100, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT, bitlength*9+1, AudioTrack.MODE_STATIC);
//        short[] trackShorts = new short[bitlength*9];
//        int i = 0;
//        int initial = 0;
//        for (i = initial; i < initial + bitlength; i++) {
//            trackShorts[i] = Short.MAX_VALUE;
//        }
//        for (int k = 0; k < 8; k++) {
//            initial = i;
//            if (val%2==0) {
//                for (; i < initial + bitlength; i++) {
//                    trackShorts[i] = 0;
//                }
//            } else {
//                for (; i < initial + bitlength; i++) {
//                    trackShorts[i] = Short.MAX_VALUE;
//                }
//            }
//            val=(val/2);
//        }
//        audiotrack.write(trackShorts, 0, bitlength*9);
//        audiotrack.play();
	}
}
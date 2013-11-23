/*
  AudioSerial.java - Audio serial library for communication from Android smartphone to Arduino
  Created by Guillaume Dupre, Friday 26, 2013.
  Released into the public domain.
*/

package com.audioserial.servocontrol;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

public class AudioSerial {
    public static final int BITS_PER_BYTE = 10;
    AudioTrack audiotrack;
    short[] low;
    short[] high;

    int bitlength;
    // bps : bits per second, 20 recommended
    public AudioSerial(int bps){
        bitlength=44100/bps;
        audiotrack= new AudioTrack(AudioManager.STREAM_MUSIC, 44100, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT, bitlength * 2, AudioTrack.MODE_STREAM);
        // Initialization of low and high waveforms
        low=new short[bitlength];
        high=new short[bitlength];
        for(int i=0;i<bitlength;i++){
            low[i]=0;
            high[i]= Short.MAX_VALUE;
        }
        audiotrack.setStereoVolume(1,1);
        audiotrack.play();
    }

    // Sending one byte starting from the least significant bit
    public void send(int val, boolean isInverted){
        // Start bit
        audiotrack.write(high, 0, bitlength);

        // Data Byte
        for(int i=0;i<8;i++){
            if(val%2== (isInverted ? 1 : 0)){
                audiotrack.write(low, 0, bitlength);
            }else{
                audiotrack.write(high, 0, bitlength);
            }
            val= val >> 1;
        }

        // Stop bit
        audiotrack.write(low, 0, bitlength);
    }
}
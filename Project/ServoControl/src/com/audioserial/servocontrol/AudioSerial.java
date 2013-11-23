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
    public static final char MESSAGE_DELIMITER = '\n';

    AudioTrack audiotrack;
    short[] low;
    short[] high;
    short[] endTrack;

    int baudrate;
    int sampleRate;
    int bitlength;
    int bufferSize;

    public AudioSerial(int baudrate, int sampleRate){
        this.baudrate = baudrate;
        this.sampleRate = sampleRate;
        bitlength=sampleRate/baudrate;
        bufferSize = AudioTrack.getMinBufferSize(sampleRate, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT);

        // Initialization of low and high waveforms
        low=new short[bitlength];
        high=new short[bitlength];
        for(int i=0;i<bitlength;i++){
            low[i]=0;
            high[i]= Short.MAX_VALUE;
        }
        // Initialization of endTrack buffer
        endTrack = new short[bufferSize];
        for (int i = 0; i < bufferSize; i++) {
            endTrack[i] = 0;
        }

        audiotrack= new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT, bufferSize, AudioTrack.MODE_STREAM);
        audiotrack.setStereoVolume(1,1);
        audiotrack.play();
    }

    // BUG: If the first bit sent after the first start bit is LOW, you can get a weird read from the microcontroller.
    public void send(String str, boolean isInverted) {
        for (int i = 0; i < str.length(); i++) {
            send(str.charAt(i), isInverted);
        }
        send(MESSAGE_DELIMITER, isInverted);

        // endTrack: This ensures all important audiotrack data is played.
        audiotrack.write(endTrack, 0, bufferSize);
    }

    // Sending one byte starting from the least significant bit
    public void send(int val, boolean isInverted) {
        // Start bit
        audiotrack.write(high, 0, bitlength);

        // Data Byte
        for(int i=0;i<8;i++){
            if(val % 2 == (isInverted ? 1 : 0)){
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
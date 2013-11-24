/*
  AudioSerial.java - Audio serial library for communication from Android smartphone to Arduino
  Created by Guillaume Dupre, Friday 26, 2013.
  Released into the public domain.
*/

package com.audioserial.servocontrol;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Handler;
import android.widget.TextView;

public class AudioSerial {
    // TX
    public static final int BITS_PER_BYTE = 10;
    public static final char MESSAGE_DELIMITER = '\n';
    // RX
    private static final int POLL_INTERVAL = 500;// every half second

    // GENERAL
    int baudrate = 300;
    int sampleRate = 44100;

    // TX
    AudioTrack audiotrack;
    short[] low;
    short[] high;
    short[] endTrack;

    // RX
    boolean stopped = true;
    int updateCount = 0;
    TextView sensorReading;
    SoundMeter mSensor = new SoundMeter();
    Handler mHandler = new Handler();
    Runnable mPollTask = new Runnable() {
        public void run() {
            int amp = mSensor.getAmplitude();

            if (sensorReading != null) {
                updateCount++;
                sensorReading.setText("Sensor Reading: " + amp + "\nUpdate " + updateCount);
            }

            if (!stopped) {
                mHandler.postDelayed(mPollTask, POLL_INTERVAL);
            }
        }
    };

    // GENERAL METHODS
    public void reset(int baudrate, int sampleRate) {
        this.baudrate = baudrate;
        this.sampleRate = sampleRate;
        resetTX();
    }

    private int bitlength() {
        return sampleRate/baudrate;
    }

    // TX METHODS
    private int TXbufferSize() {
        return AudioTrack.getMinBufferSize(sampleRate, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT);
    }

    private void resetTX() {
        int bitlength = bitlength();
        int TXbufferSize = TXbufferSize();

        // Initialization of low and high waveforms
        low=new short[bitlength];
        high=new short[bitlength];
        for(int i=0;i<bitlength;i++){
            low[i]=0;
            high[i]= Short.MAX_VALUE;
        }
        // Initialization of endTrack buffer
        endTrack = new short[TXbufferSize];
        for (int i = 0; i < TXbufferSize; i++) {
            endTrack[i] = 0;
        }

        audiotrack= new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT, TXbufferSize, AudioTrack.MODE_STREAM);
        audiotrack.setStereoVolume(1,1);
        audiotrack.play();
    }

    // BUG: If the first bit sent after the first start bit is LOW, you can get a weird read from the microcontroller.
    public void send(String str, boolean isInverted) {
        int TXbufferSize = TXbufferSize();

        for (int i = 0; i < str.length(); i++) {
            send(str.charAt(i), isInverted);
        }
        send(MESSAGE_DELIMITER, isInverted);

        // endTrack: This ensures all important audiotrack data is played.
        audiotrack.write(endTrack, 0, TXbufferSize);
    }

    // Sending one byte starting from the least significant bit
    public void send(int val, boolean isInverted) {
        int bitlength = bitlength();

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

    // RX METHODS
    public void resetRX() {

    }

    public void startRX() {
        if (stopped) {
            stopped = false;
            mSensor.start();
            mHandler.postDelayed(mPollTask, POLL_INTERVAL);
        }
    }

    public void stopRX() {
        if (!stopped) {
            stopped = true;
            mHandler.removeCallbacks(mPollTask);
            mSensor.stop();
        }
    }

}
/*
  AudioSerial.java - Audio serial library for communication from Android smartphone to Arduino
  Created by Guillaume Dupre, Friday 26, 2013.
  Released into the public domain.
*/

package com.audioserial.servocontrol;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioRecord;
import android.media.AudioTrack;
import android.media.MediaRecorder;
import android.os.Handler;
import android.widget.TextView;

public class AudioSerial {
    // GENERAL
    public static AudioSerial singleton;

    public static final int BITS_PER_BYTE = 10;
    public static final char MESSAGE_DELIMITER = '\n';
    public static final int MAX_MESSAGE_SIZE = 10;
    public static final int NO_INDEX_FOUND = -1;

    int sampleRate = 44100;

    // Debugging
    TextView customMessageTextView;
    TextView sensorReading;

    // TX
    int baudrateTX;
    AudioTrack audiotrack;
    short[] low;
    short[] high;
    short[] endTrack;

    // RX
    // Special Protocol: receive one second start bit, then stop bit, then packet.
    private static final int MIC_SERIAL_THRESHOLD = 10000;
    private static final int RX_POLL_INTERVAL = 500;// every half second

    int baudrateRX;
    SoundMeter micAmplitudeSensor = new SoundMeter();
    AudioRecord audiorecord = null;

    boolean isPollingRX = false;
    int pollCountRX = 0;
    int bufferSizeRX;
    short[] bufferRX;

    Handler handlerRX = new Handler();
    Runnable pollTaskRX = new Runnable() {
        public void run() {
            pollTaskRun();
        }
    };

    // GENERAL METHODS
    public void reset(int baudrateTX, int baudrateRX, int sampleRate) {
        this.baudrateTX = baudrateTX;
        this.baudrateRX = baudrateRX;
        this.sampleRate = sampleRate;
        resetTX();
        resetRX();
    }

    // TX METHODS
    private int bitlengthTX() {
        return sampleRate/baudrateTX;
    }

    private int TXbufferSize() {
        return AudioTrack.getMinBufferSize(sampleRate, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT);
    }

    private void resetTX() {
        int bitlength = bitlengthTX();
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
        int bitlength = bitlengthTX();

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
    public int bitlengthRX() {
        return sampleRate / baudrateRX;
    }

    public void resetRX() {
        pollCountRX = 0;

        bufferSizeRX =
            (sampleRate * 1) +                                              // one second start bit
            (sampleRate * 1 / baudrateRX) +                                 // one stop bit
            (sampleRate * (BITS_PER_BYTE * MAX_MESSAGE_SIZE) / baudrateRX); // max bytes in a message

        int minBufferSize = AudioRecord.getMinBufferSize(sampleRate, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);
        // Ensure buffer size is large enough to instantiate AudioRecord. Since bufferRX is shorts, this will actually be at least twice the minimum size.
        bufferSizeRX = bufferSizeRX < minBufferSize ? minBufferSize : bufferSizeRX;

        bufferRX = new short[bufferSizeRX];
    }

    public void startRX() {
        if (!isPollingRX) {
            isPollingRX = true;
            micAmplitudeSensor.start();
            handlerRX.postDelayed(pollTaskRX, RX_POLL_INTERVAL);
        }
    }

    public void stopRX() {
        if (isPollingRX) {
            isPollingRX = false;
            handlerRX.removeCallbacks(pollTaskRX);
            micAmplitudeSensor.stop();
        }
    }

    public void pollTaskRun() {
        pollCountRX++;

        int micPolledAmplitude = micAmplitudeSensor.getAmplitude();

        if (micPolledAmplitude > MIC_SERIAL_THRESHOLD) {
            receivePacket(); // long operation
        }

        if (sensorReading != null) {
            sensorReading.setText("Sensor Reading: " + micPolledAmplitude + "\nUpdate " + pollCountRX);
        }

        if (isPollingRX) {
            handlerRX.postDelayed(pollTaskRX, RX_POLL_INTERVAL);
        }
    }

    public void receivePacket() {
        audiorecord = new AudioRecord(MediaRecorder.AudioSource.MIC,
                                      sampleRate, 
                                      AudioFormat.CHANNEL_IN_MONO, 
                                      AudioFormat.ENCODING_PCM_16BIT, bufferSizeRX * 2); // multiply bufferSizeRX by 2 since samples are shorts, not bytes

        micAmplitudeSensor.stop(); // also releases resources
        audiorecord.startRecording();
        // Record message
        audiorecord.read(bufferRX, 0, bufferRX.length); // blocks until read finishes
        audiorecord.stop();
        audiorecord.release();
        audiorecord = null;
        micAmplitudeSensor.start();

        // Parse message
        parsePacket();
    }

    public void parsePacket() {
        String customMessage = "Parsed Packet\n";
        String packetString = "";

        int[] runningAverageMagnitude = computeRunningAverageMagnitude();

        int bufferIndex = 0;

        // Find the first stop bit
        bufferIndex = nextBitIndex(runningAverageMagnitude, bufferIndex, false);
        if (bufferIndex == NO_INDEX_FOUND) {
            // No stop bit found. Do not continue parsing
            System.out.println("No stop bit found");
            return;
        }

        customMessage += bufferIndex + " first stop bit " + runningAverageMagnitude[bufferIndex] + "\n";
        bufferIndex += bitlengthRX(); // shouldn't be necessary, but it seems to help with the first byte's reading
        // Parse Packet
        for (int packetIndex = 0; packetIndex < MAX_MESSAGE_SIZE; packetIndex++) {
            // Find the start bit
            bufferIndex = nextBitIndex(runningAverageMagnitude, bufferIndex, true);
            if (bufferIndex == NO_INDEX_FOUND) {
                // No start bit found. Do not continue parsing
                System.out.println("No start bit found " + packetIndex);
                System.out.println(customMessage);
                return;
            }

            if (packetIndex == 0) {
                customMessage += bufferIndex + " start bit " + runningAverageMagnitude[bufferIndex] + "\n";
            }

            // Offset to the middle of the bit
            bufferIndex += bitlengthRX() / 2;
            // Offset to the first bit
            bufferIndex += bitlengthRX();

            // Read byte
            char c = 0;
            for (int bitIndex = 0; bitIndex < 8; bitIndex++) {
                int bit = runningAverageMagnitude[bufferIndex] > MIC_SERIAL_THRESHOLD ? 1 : 0;
                c = (char) (c | (bit << bitIndex));
                if (packetIndex == 0) {
                    customMessage += runningAverageMagnitude[bufferIndex] + " ";
                    customMessageTextView.setText(customMessage);
                }
                bufferIndex += bitlengthRX();
            }
            if (packetIndex == 0) {
                customMessage += "\n";
            }

            // If found message delimiter character, stop parsing
            System.out.println(c);
            if (c == MESSAGE_DELIMITER) {
                packetString+="\\n";
                break;
            }
            packetString += c;
        }
        customMessage += packetString;

        customMessageTextView.setText(customMessage);
    }

    private int[] computeRunningAverageMagnitude() {
        // Compute running average of bufferRX
        int bitlengthAveragingFactor = 10; // somewhat arbitrary, should be > 2
        int numSamplesInAverage = bitlengthRX() / bitlengthAveragingFactor;
        int[] runningAverageMagnitude = new int[bufferRX.length - numSamplesInAverage];
        for (int i = 0; i < runningAverageMagnitude.length; i++) {
            runningAverageMagnitude[i] = 0;
            for (int j = 0; j < numSamplesInAverage; j++) {
                runningAverageMagnitude[i] += Math.abs(bufferRX[i+j]);
            }
            runningAverageMagnitude[i] = runningAverageMagnitude[i] / numSamplesInAverage;
        }
        return runningAverageMagnitude;
    }

    private int nextBitIndex(int[] runningAverageMagnitude, int initialBufferIndex, boolean lookingForHighBit) {
        int bufferIndex = initialBufferIndex;

        while (true) {
            if (bufferIndex >= runningAverageMagnitude.length) {
                return NO_INDEX_FOUND;
            }

            boolean isHighBit = runningAverageMagnitude[bufferIndex] > MIC_SERIAL_THRESHOLD;
            if (isHighBit && lookingForHighBit) {
                return bufferIndex;
            }
            if (!isHighBit && !lookingForHighBit) {
                return bufferIndex;
            }

            bufferIndex++;
        }
    }
}
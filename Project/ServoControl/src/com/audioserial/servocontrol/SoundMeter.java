package com.audioserial.servocontrol;

/*
 * Based on Google NoiseAlert by Bernhard.R.Suter@gmail.com
 * https://code.google.com/p/android-labs/wiki/NoiseAlert
 */

import java.io.IOException;

import android.media.MediaRecorder;

public class SoundMeter {

        private MediaRecorder mRecorder = null;

        public void start() {
                if (mRecorder == null) {

                    mRecorder = new MediaRecorder();
                    mRecorder.setAudioSource(MediaRecorder.AudioSource.MIC);
                    mRecorder.setOutputFormat(MediaRecorder.OutputFormat.THREE_GPP);
                    mRecorder.setAudioEncoder(MediaRecorder.AudioEncoder.AMR_NB);
                    mRecorder.setOutputFile("/dev/null");
                    try {
                        mRecorder.prepare();
                    } catch (IllegalStateException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    } catch (IOException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                    mRecorder.start();
                    mRecorder.getMaxAmplitude(); // to get rid of initial 0 return value
                }
        }
        public void stop() {
                if (mRecorder != null) {
                        mRecorder.stop();
                        mRecorder.release();
                        mRecorder = null;
                }
        }

        public int getAmplitude() {
                if (mRecorder != null)
                        return  (mRecorder.getMaxAmplitude());
                else
                        return 0;

        }
}
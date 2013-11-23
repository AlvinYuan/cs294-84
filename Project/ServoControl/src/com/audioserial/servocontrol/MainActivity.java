package com.audioserial.servocontrol;

import android.media.AudioFormat;
import android.media.AudioTrack;
import android.os.Bundle;
import android.os.Handler;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;

public class MainActivity extends Activity {
    private static final int POLL_INTERVAL = 3000;//1000 / BAUD_RATE / 4;
    private static final int AUDIO_SERIAL_RECEIVE_AMPLITUDE_THRESHOLD = 300;

    Button dangerButton, sosButton, resetButton;
    TextView sensorReading, micTextView, baudRateEditText, sampleRateEditText, bufferSizeTextView;

    AudioSerial audioserial;
    boolean stopped;
    int updateCount = 0;

    private SoundMeter mSensor = new SoundMeter();
    private Handler mHandler = new Handler();
    private Runnable mPollTask = new Runnable() {
        public void run() {
            int amp = mSensor.getAmplitude();

            updateCount++;
            sensorReading.setText("Sensor Reading: " + amp + "\nUpdate " + updateCount);
            if (!stopped) {
                mHandler.postDelayed(mPollTask, POLL_INTERVAL);
            }
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);


        sensorReading = (TextView) findViewById(R.id.sensorReadingTextView);
        micTextView = (TextView) findViewById(R.id.micTextView);
        baudRateEditText = (TextView) findViewById(R.id.BaudRateEditText);
        sampleRateEditText = (TextView) findViewById(R.id.SampleRateEditText);
        bufferSizeTextView = (TextView) findViewById(R.id.BufferSizeTextView);

        reset();

        dangerButton = (Button) findViewById(R.id.DangerButton);
        dangerButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                audioserial.send(" ABCDEFGHIJKLMNOPQRSTUVWXYZ", true);
            }
        });

        sosButton = (Button) findViewById(R.id.SOSButton);
        sosButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                audioserial.send(" SOS", true);
            }
        });

        resetButton = (Button) findViewById(R.id.resetButton);
        resetButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                reset();
            }
        });
}

    //http://stackoverflow.com/questions/14708636/detect-whether-headset-has-microphone
    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            if (Intent.ACTION_HEADSET_PLUG.equals(action)) {
                Log.d("HeadSetPlugInTest", "state: " + intent.getIntExtra("state", -1));
                Log.d("HeadSetPlugInTest", "microphone: " + intent.getIntExtra("microphone", -1));
                if (intent.getIntExtra("state", -1) != 1) {
                    micTextView.setText("No AUDIO CONNECTION FOUND");
                } else if (intent.getIntExtra("microphone", -1) == 1) {
                    micTextView.setText("MIC FOUND");
                } else {
                    micTextView.setText("NO MIC FOUND");
                }
            }
        }
    };

    @Override
    public void onResume() {
        super.onResume();
        IntentFilter filter = new IntentFilter(Intent.ACTION_HEADSET_PLUG);
        getApplicationContext().registerReceiver(mReceiver, filter);

        stopped = false;
        start();
    }

    @Override
    public void onStop() {
        super.onStop();
        getApplicationContext().unregisterReceiver(mReceiver);

        stopped = true;
        stop();
    }

    private void start() {
        mSensor.start();
        mHandler.postDelayed(mPollTask, POLL_INTERVAL);
    }

    private void stop() {
        mHandler.removeCallbacks(mPollTask);
        mSensor.stop();
    }

    void reset() {
        int baudRate = Integer.parseInt(baudRateEditText.getText().toString());
        int sampleRate = Integer.parseInt(sampleRateEditText.getText().toString());
        int bufferSize = AudioTrack.getMinBufferSize(sampleRate, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT);
        if (bufferSize > 0) {
            bufferSizeTextView.setText("Min Buffer Size = " + bufferSize);
            audioserial=new AudioSerial(baudRate, sampleRate);
        } else {
            bufferSizeTextView.setText("Got invalid buffer size");
        }
    }
}

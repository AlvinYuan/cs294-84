package com.audioserial.servocontrol;

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
    private static final int BAUD_RATE = 20;
    private static final int POLL_INTERVAL = 300;//1000 / BAUD_RATE / 4;
    private static final int AUDIO_SERIAL_RECEIVE_AMPLITUDE_THRESHOLD = 500;

    Button releaseButton, resetButton;
    TextView textview, sensorReading, micTextView, doorTextView;

    AudioSerial audioserial;
    boolean stopped;
    int updateCount = 0;

    private SoundMeter mSensor = new SoundMeter();
    private Handler mHandler = new Handler();
    private Runnable mPollTask = new Runnable() {
        public void run() {
            int amp = mSensor.getAmplitude();
            if (amp > AUDIO_SERIAL_RECEIVE_AMPLITUDE_THRESHOLD) {
                doorTextView.setText("Door is open");
            } else {
                doorTextView.setText("Door is closed");
            }

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

        textview = (TextView) findViewById(R.id.textView1);
        sensorReading = (TextView) findViewById(R.id.sensorReadingTextView);
        micTextView = (TextView) findViewById(R.id.micTextView);
        doorTextView = (TextView) findViewById(R.id.doorTextView);

        // Opening AudioSerial communication at 20 bits per second and with starting byte 138
        reset();

        releaseButton = (Button) findViewById(R.id.releaseButton);
        releaseButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                textview.setText("Congratulations. You came in like a wrecking ball.");
                audioserial.send('R');
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
        audioserial=new AudioSerial(BAUD_RATE,138);
    }
}

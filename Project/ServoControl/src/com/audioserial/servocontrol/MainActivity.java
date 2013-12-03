package com.audioserial.servocontrol;

import android.media.AudioFormat;
import android.media.AudioTrack;
import android.os.Bundle;
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
    Button dangerButton, sosButton, resetButton;
    TextView sensorReading, micTextView, baudRateEditText, sampleRateEditText, bufferSizeTextView, customMessageTextView;

    AudioSerial audioserial = new AudioSerial();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);


        sensorReading = (TextView) findViewById(R.id.sensorReadingTextView);
        micTextView = (TextView) findViewById(R.id.micTextView);
        baudRateEditText = (TextView) findViewById(R.id.BaudRateEditText);
        sampleRateEditText = (TextView) findViewById(R.id.SampleRateEditText);
        bufferSizeTextView = (TextView) findViewById(R.id.BufferSizeTextView);
        customMessageTextView = (TextView) findViewById(R.id.CustomMessageTextView);

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

    @Override
    public void onResume() {
        super.onResume();
        // For determining whether audio jack / microphone is plugged into phone.
        IntentFilter filter = new IntentFilter(Intent.ACTION_HEADSET_PLUG);
        getApplicationContext().registerReceiver(mReceiver, filter);

        audioserial.startRX();
    }

    @Override
    public void onStop() {
        super.onStop();
        getApplicationContext().unregisterReceiver(mReceiver);

        audioserial.stopRX();
    }

    void reset() {
        int baudRate = Integer.parseInt(baudRateEditText.getText().toString());
        int sampleRate = Integer.parseInt(sampleRateEditText.getText().toString());
        int bufferSize = AudioTrack.getMinBufferSize(sampleRate, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT);
        if (bufferSize > 0) {
            bufferSizeTextView.setText("Min Buffer Size = " + bufferSize);
            audioserial.reset(baudRate, 20, sampleRate);
            audioserial.sensorReading = sensorReading;
            audioserial.customMessageTextView = customMessageTextView;
        } else {
            bufferSizeTextView.setText("Got invalid buffer size");
        }
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

}

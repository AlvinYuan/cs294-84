package com.audioserial.servocontrol;

import android.R;
import android.media.AudioFormat;
import android.media.AudioTrack;
import android.os.Bundle;
import android.app.Activity;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;


public class Send_message_actvity extends Activity {
	Button sendMessageButton, seeAlertsLog, seeAlertsMap;
    TextView sensorReading, micTextView, baudRateEditText, sampleRateEditText, bufferSizeTextView;
    RadioButton fire, eq, flood, chem, gun, other;
    
    AudioSerial audioserial = new AudioSerial();;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_send_message_actvity);
        sensorReading = (TextView) findViewById(R.id.sensorReadingTextView);
        micTextView = (TextView) findViewById(R.id.micTextView);
        baudRateEditText = (TextView) findViewById(R.id.BaudRateEditText);
        sampleRateEditText = (TextView) findViewById(R.id.SampleRateEditText);
        bufferSizeTextView = (TextView) findViewById(R.id.BufferSizeTextView);
        fire = (RadioButton) findViewById(R.id.fire);
        eq = (RadioButton) findViewById(R.id.eq);
        flood = (RadioButton) findViewById(R.id.flood);
        chem = (RadioButton) findViewById(R.id.chem);
        gun = (RadioButton) findViewById(R.id.gun);
        other = (RadioButton) findViewById(R.id.fire);
        
        reset();

		sendMessageButton = (Button) findViewById(R.id.SendMessageButton);
		sendMessageButton.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                audioserial.send(" ABCDEFGHIJKLMNOPQRSTUVWXYZ", true);
            }
        });
	}
	
	void reset() {
        int baudRate = Integer.parseInt(baudRateEditText.getText().toString());
        int sampleRate = Integer.parseInt(sampleRateEditText.getText().toString());
        int bufferSize = AudioTrack.getMinBufferSize(sampleRate, AudioFormat.CHANNEL_OUT_MONO, AudioFormat.ENCODING_PCM_16BIT);
        if (bufferSize > 0) {
            bufferSizeTextView.setText("Min Buffer Size = " + bufferSize);
            audioserial.reset(baudRate, sampleRate);
            audioserial.sensorReading = sensorReading;
        } else {
            bufferSizeTextView.setText("Got invalid buffer size");
        }
    }

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.send_message_actvity, menu);
		return true;
	}

}

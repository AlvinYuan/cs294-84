package com.audioserial.servocontrol;

import android.os.Bundle;
import android.app.Activity;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;

public class MainActivity extends Activity {
	Button button1,button2,button3;
	int servoposition;
	AudioSerial audioserial;
	TextView textview;
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		// Opening AudioSerial communication at 20 bits per second and with starting byte 138
		audioserial=new AudioSerial(20,138);
//		audioserial.send(0); // Set servo starting position to 0  
		servoposition=0;

		textview = (TextView) findViewById(R.id.textView1);

		button1 = (Button) findViewById(R.id.button1);

		button1.setOnClickListener(new OnClickListener() {
			public void onClick(View v) {

				if(servoposition<180){
					servoposition+=1;
					textview.setText("Servo Position : "+servoposition);
					audioserial.send('a' + servoposition);
				}
			}
		});
		button2 = (Button) findViewById(R.id.button2);
		button2.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				if(servoposition>0){
					servoposition-=1;
					textview.setText("Servo Position : "+servoposition);
					audioserial.send('a' + servoposition);
				}
			}
		});
		button3 = (Button) findViewById(R.id.button3);
		button3.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				audioserial=new AudioSerial(20,138);
//				audioserial.send(0); // Set servo starting position to 0  
				servoposition=0;
				textview.setText("Servo Position : "+servoposition);
				
			}
		});
	}



}

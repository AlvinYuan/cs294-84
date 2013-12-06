package com.audioserial.servocontrol;

import android.media.AudioFormat;
import android.media.AudioTrack;
import android.os.Bundle;
import android.app.Activity;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.TextView.OnEditorActionListener;
import android.widget.RadioGroup.OnCheckedChangeListener;

public class Send_message_actvity extends Activity {
	TextView alerts, what, lvl;
	Button sendMessageButton, seeAlertsLog, seeAlertsMap;
    Button fire, eq, flood, chem, gun, other;
    String s;
    RadioGroup g1, g2;
    AudioSerial audioserial = new AudioSerial();;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_send_message_actvity);
		alerts = (TextView) findViewById(R.id.AlertsHeader);
		what = (TextView) findViewById(R.id.WhatTxt);
		lvl = (TextView) findViewById(R.id.DangerTxt);
		
        fire = (Button) findViewById(R.id.fire);
        eq = (Button) findViewById(R.id.eq);
        flood = (Button) findViewById(R.id.flood);
        chem = (Button) findViewById(R.id.chem);
        gun = (Button) findViewById(R.id.gun);
        other = (Button) findViewById(R.id.other);
        //what the danger is, fire, flood?
        g1 = (RadioGroup) findViewById(R.id.radioGroup1);
        g1.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			
			public void onCheckedChanged(RadioGroup group, int checkedId) {
					s.concat("id ");
			}
		});
		
        g2 = (RadioGroup) findViewById(R.id.radioGroup2);
        g2.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			
			public void onCheckedChanged(RadioGroup group, int checkedId) {
					s.concat("id ");
			}
		});
        
        //what the lvl of danger is?
        SeekBar sb = (SeekBar) findViewById(R.id.DangerLvlBar);
        sb.setOnSeekBarChangeListener( new OnSeekBarChangeListener(){

			@Override
			public void onProgressChanged(SeekBar arg0, int arg1, boolean arg2) {
				// TODO Auto-generated method stub
				s.concat(Integer.toString(arg1)+" ");
			}

			@Override
			public void onStartTrackingTouch(SeekBar arg0) {
				// TODO Auto-generated method stub
			}

			@Override
			public void onStopTrackingTouch(SeekBar arg0) {
				// TODO Auto-generated method stub				
			}
        });
        
        //what details user added?
        EditText et = (EditText) findViewById(R.id.Details);
        et.setOnEditorActionListener(new OnEditorActionListener() {
        	public boolean onEditorAction(TextView v, int actionID, KeyEvent event){
        		if (actionID == EditorInfo.IME_ACTION_SEND){
        			s.concat(v.toString() );
        		}
        		return true;
        	}
        });
        
        Switch sos = (Switch) findViewById(R.id.SOS);
        sos.setOnClickListener(new OnClickListener() {
        	
        	public void onClick(View v){
        		audioserial.send(" SOS /n", true);
        	}
        });
        
		sendMessageButton = (Button) findViewById(R.id.SendMsg);
		sendMessageButton.setOnClickListener(new OnClickListener() {
			
            public void onClick(View v) {
            	String f = "D ";
            	f.concat(s);
            	f.concat("/n");
                audioserial.send(f, true);
            }
        });
		
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.send_message_actvity, menu);
		return true;
	}

}

package com.audioserial.servocontrol;

import android.os.Bundle;
import android.app.Activity;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RadioGroup;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.RadioGroup.OnCheckedChangeListener;

public class Send_message_actvity extends Activity {
    TextView details;
    int seekbarProgress = 0;
    String dangerChecked = " ";
    Button sendMessageButton, seeAlertsLog, seeAlertsMap;
//    Button fire, eq, flood, chem, gun, other;
    RadioGroup g1, g2;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_send_message_actvity);
        details = (EditText) findViewById(R.id.Details);

//        fire = (Button) findViewById(R.id.fire);
//        eq = (Button) findViewById(R.id.eq);
//        flood = (Button) findViewById(R.id.flood);
//        chem = (Button) findViewById(R.id.chem);
//        gun = (Button) findViewById(R.id.gun);
//        other = (Button) findViewById(R.id.other);
        //what the danger is, fire, flood?
        g1 = (RadioGroup) findViewById(R.id.radioGroup1);
        g1.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            public void onCheckedChanged(RadioGroup group, int checkedId) {
            	if (checkedId == R.id.fire) {
            		dangerChecked = "fire" + " ";
            	}
            	else if (checkedId == R.id.eq){
            		dangerChecked = "eq" + " ";
            	}
            	else if (checkedId == R.id.gun){
            		dangerChecked = "gun" + " ";
            	}
            }
        });

        g2 = (RadioGroup) findViewById(R.id.radioGroup2);
        g2.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            public void onCheckedChanged(RadioGroup group, int checkedId) {
            	if (checkedId == R.id.chem) {
            		dangerChecked = "chem" + " ";
            	}
            	else if (checkedId == R.id.flood){
            		dangerChecked = "flood" + " ";
            	}
            	else if (checkedId == R.id.other){
            		dangerChecked = "other" + " ";
            	}
            }
        });

        //what the lvl of danger is?
        SeekBar sb = (SeekBar) findViewById(R.id.DangerLvlBar);
        sb.setOnSeekBarChangeListener( new OnSeekBarChangeListener(){

            @Override
            public void onProgressChanged(SeekBar arg0, int arg1, boolean arg2) {
                // TODO Auto-generated method stub
                seekbarProgress = arg1;
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
        
        Switch sos = (Switch) findViewById(R.id.SOS);
        sos.setOnClickListener(new OnClickListener() {

            public void onClick(View v){
                AudioSerial.singleton.send(" SOS \n", true);
            }
        });

        sendMessageButton = (Button) findViewById(R.id.SendMsg);
        sendMessageButton.setOnClickListener(new OnClickListener() {

        	public void onClick(View v) {
                String f = "D "+ dangerChecked + " " + seekbarProgress + " " + details.getText().toString() + " ";
                f += AudioSerial.MESSAGE_DELIMITER;
                System.out.println(f);
                AudioSerial.singleton.send(f, true);
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

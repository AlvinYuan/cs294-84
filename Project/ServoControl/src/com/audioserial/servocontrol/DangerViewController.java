package com.audioserial.servocontrol;

import android.app.Activity;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.RadioGroup;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import android.widget.RadioGroup.OnCheckedChangeListener;

public class DangerViewController {
    TextView details;
    Packet.TypeOfDanger dangerType = Packet.TypeOfDanger.NOT_SPECIFIED;
    Packet.LevelOfDanger dangerLevel = Packet.LevelOfDanger.PROCEED_WITH_CAUTION;
    Button sendMessageButton, seeAlertsLog, seeAlertsMap;
    RadioGroup g1, g2;

    public DangerViewController(View dangerView, Activity activity) {
        details = (TextView) dangerView.findViewById(R.id.Details);

        //what the danger is, fire, flood?
        g1 = (RadioGroup) dangerView.findViewById(R.id.radioGroup1);
        g1.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            public void onCheckedChanged(RadioGroup group, int checkedId) {
                if (checkedId == R.id.fire) {
                    dangerType = Packet.TypeOfDanger.FIRE;
                }
                else if (checkedId == R.id.unstable){
                    // replace this option? for the sake of focusing on one type of emergency (disaster)
                    dangerType = Packet.TypeOfDanger.NOT_SPECIFIED;
                }
            }
        });

        g2 = (RadioGroup) dangerView.findViewById(R.id.radioGroup2);
        g2.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            public void onCheckedChanged(RadioGroup group, int checkedId) {
                if (checkedId == R.id.chem) {
                    dangerType = Packet.TypeOfDanger.CHEMICAL;
                }
                else if (checkedId == R.id.other){
                    dangerType = Packet.TypeOfDanger.NOT_SPECIFIED;
                }
            }
        });

        //what the lvl of danger is?
        SeekBar sb = (SeekBar) dangerView.findViewById(R.id.DangerLvlBar);
        sb.setOnSeekBarChangeListener( new OnSeekBarChangeListener(){

            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                // TODO Auto-generated method stub
                switch (progress) {
                case 0:
                    dangerLevel = Packet.LevelOfDanger.PROCEED_WITH_CAUTION;
                    break;
                case 1:
                    dangerLevel = Packet.LevelOfDanger.PROCEED_WITH_CAUTION;
                    break;
                case 2:
                    dangerLevel = Packet.LevelOfDanger.GENERAL_DANGER;
                    break;
                case 3:
                    dangerLevel = Packet.LevelOfDanger.EVACUATE_THE_AREA;
                    break;
                default:
                    dangerLevel = Packet.LevelOfDanger.GENERAL_DANGER;
                    break;
                }
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

        sendMessageButton = (Button) dangerView.findViewById(R.id.SendMsg);
        sendMessageButton.setOnClickListener(new OnClickListener() {

            public void onClick(View v) {
                AudioSerial.singleton.send(new Packet(Packet.PacketType.DANGER, dangerType, dangerLevel, details.getText().toString()), true);
            }
        });

    }

}
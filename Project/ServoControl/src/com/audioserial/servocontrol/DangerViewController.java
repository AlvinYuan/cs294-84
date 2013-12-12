package com.audioserial.servocontrol;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.graphics.Point;
import android.view.Display;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.RadioGroup;
import android.widget.RadioButton;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import android.widget.RadioGroup.OnCheckedChangeListener;

public class DangerViewController implements OnSeekBarChangeListener {
    TextView details;
    Packet.TypeOfDanger dangerType = Packet.TypeOfDanger.NOT_SPECIFIED;
    Packet.LevelOfDanger dangerLevel = Packet.LevelOfDanger.PROCEED_WITH_CAUTION;
    Button sendMessageButton, seeAlertsLog, seeAlertsMap;
    RadioGroup g1, g2;
    TextView lowDangerTextView, mediumDangerTextView, highDangerTextView;

    @SuppressLint("NewApi")
    public DangerViewController(View dangerView, Activity activity) {
        details = (TextView) dangerView.findViewById(R.id.Details);

        //what the danger is, fire, flood?
        g1 = (RadioGroup) dangerView.findViewById(R.id.radioGroup1);
        g1.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            public void onCheckedChanged(RadioGroup group, int checkedId) {
                if (checkedId != -1 && ((RadioButton) g1.findViewById(checkedId)).isChecked()) {
                    g2.clearCheck();
                    if (checkedId == R.id.fire) {
                        dangerType = Packet.TypeOfDanger.FIRE;
                    }
                    else if (checkedId == R.id.unstable){
                        // replace this option? for the sake of focusing on one type of emergency (disaster)
                        dangerType = Packet.TypeOfDanger.UNSTABLE_SURROUNDINGS;
                    }
                }
            }
        });

        g2 = (RadioGroup) dangerView.findViewById(R.id.radioGroup2);
        g2.setOnCheckedChangeListener(new OnCheckedChangeListener() {

            public void onCheckedChanged(RadioGroup group, int checkedId) {
                if (checkedId != -1 && ((RadioButton) g2.findViewById(checkedId)).isChecked()) {
                    g1.clearCheck();
                    if (checkedId == R.id.chem) {
                        dangerType = Packet.TypeOfDanger.CHEMICAL;
                    }
                    else if (checkedId == R.id.other){
                        dangerType = Packet.TypeOfDanger.NOT_SPECIFIED;
                    }
                }
            }
        });

        //what the lvl of danger is?
        SeekBar sb = (SeekBar) dangerView.findViewById(R.id.DangerLvlBar);
        /* Fix Layout to look nice with text */
        int padding = sb.getPaddingLeft();
        Display display = activity.getWindowManager().getDefaultDisplay();
        Point size = new Point();
        if(android.os.Build.VERSION.SDK_INT < android.os.Build.VERSION_CODES.HONEYCOMB_MR2) {
            size.set(display.getWidth(), display.getHeight());
        } else {
            display.getSize(size);
        }
        int width = size.x;
        int margin = width * 1/6 - padding;
        RelativeLayout.LayoutParams params = (RelativeLayout.LayoutParams) sb.getLayoutParams();
        params.setMargins(margin, 0, margin, 0);
        sb.setLayoutParams(params);

        lowDangerTextView = (TextView) dangerView.findViewById(R.id.textView1);
        mediumDangerTextView = (TextView) dangerView.findViewById(R.id.textView2);
        highDangerTextView = (TextView) dangerView.findViewById(R.id.textView3);

        sb.setOnSeekBarChangeListener(this);
        sb.setProgress(1);

        sendMessageButton = (Button) dangerView.findViewById(R.id.SendMsgButton);
        sendMessageButton.setOnClickListener(new OnClickListener() {

            public void onClick(View v) {
                AudioSerial.singleton.send(new Packet(Packet.PacketType.DANGER, dangerType, dangerLevel, details.getText().toString()), true);
            }
        });

    }
    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
        // TODO Auto-generated method stub
        switch (progress) {
        case 0:
            dangerLevel = Packet.LevelOfDanger.PROCEED_WITH_CAUTION;
            lowDangerTextView.setTextColor(0xffffcc22);
            mediumDangerTextView.setTextColor(0xff000000);
            highDangerTextView.setTextColor(0xff000000);
            break;
        case 2:
            dangerLevel = Packet.LevelOfDanger.EVACUATE_THE_AREA;
            lowDangerTextView.setTextColor(0xff000000);
            mediumDangerTextView.setTextColor(0xff000000);
            highDangerTextView.setTextColor(0xffcc0000);
            break;
        default:
            dangerLevel = Packet.LevelOfDanger.GENERAL_DANGER;
            lowDangerTextView.setTextColor(0xff000000);
            mediumDangerTextView.setTextColor(0xffff8800);
            highDangerTextView.setTextColor(0xff000000);
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

}

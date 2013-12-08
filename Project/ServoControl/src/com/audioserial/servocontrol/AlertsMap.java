package com.audioserial.servocontrol;

import com.google.android.gms.maps.GoogleMap;

import android.os.Bundle;
import android.app.Activity;
import android.view.Menu;

public class AlertsMap extends Activity {
    GoogleMap map;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_alerts_map);

    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.alerts_map, menu);
        return true;
    }

}

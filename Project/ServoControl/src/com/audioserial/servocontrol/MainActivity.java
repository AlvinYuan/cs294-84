package com.audioserial.servocontrol;

import android.location.Criteria;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.support.v4.app.FragmentActivity;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends FragmentActivity implements LocationListener {
    public static Context genericContext;
    public static Location currentLocation;

    // Debugging
    public static TextView debuggingMessageTextView;

    // Simulate ActionBar tabs b/c we want to support Froyo and sherlock is ugly.
    final int numTabs = 4;
    int activeTabIndex = 0;
    View tabButtons[] = new View[numTabs];
    View tabHighlights[] = new View[numTabs];
    View tabContents[] = new View[numTabs];
    SOSViewController sosViewController;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        tabButtons[0] = findViewById(R.id.DangerTab);
        tabButtons[1] = findViewById(R.id.SOSTab);
        tabButtons[2] = findViewById(R.id.LogTab);
        tabButtons[3] = findViewById(R.id.MapTab);
        tabHighlights[0] = findViewById(R.id.SelectedTabHighlight1);
        tabHighlights[1] = findViewById(R.id.SelectedTabHighlight2);
        tabHighlights[2] = findViewById(R.id.SelectedTabHighlight3);
        tabHighlights[3] = findViewById(R.id.SelectedTabHighlight4);

        genericContext = this;
        if (AudioSerial.singleton == null) {
            AudioSerial.singleton = new AudioSerial();
            AudioSerial.singleton.reset(300, 200, 44100);
        }
        if (Packet.packetAdapter == null) {
            Packet.packetAdapter = new PacketAdapter(genericContext, android.R.layout.simple_list_item_1, Packet.packetsReceived);
        }
        LocationManager manager = (LocationManager) getSystemService(Context.LOCATION_SERVICE);
        String provider = manager.getBestProvider(new Criteria(), true);
        if (provider != null) {
            manager.requestLocationUpdates(provider, 0, 0, this);
            onLocationChanged(manager.getLastKnownLocation(provider));
        }

        LayoutInflater inflater = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        ViewGroup tabContentParent = (ViewGroup) findViewById(R.id.ContainerView);
        // Danger
        tabContents[0] = inflater.inflate(R.layout.danger_view, tabContentParent, false);
        tabContentParent.addView(tabContents[0]);
        new DangerViewController(tabContents[0], this);
        // SOS
        tabContents[1] = inflater.inflate(R.layout.sos_view, tabContentParent, false);
        tabContents[1].setVisibility(View.INVISIBLE);
        tabContentParent.addView(tabContents[1]);
        sosViewController = new SOSViewController(tabContents[1], this);
        // Log
        tabContents[2] = inflater.inflate(R.layout.log_view, tabContentParent, false);
        tabContents[2].setVisibility(View.INVISIBLE);
        tabContentParent.addView(tabContents[2]);
        ListView logListView = (ListView) findViewById(R.id.LogListView);
        logListView.setAdapter(Packet.packetAdapter);
        // Map
        tabContents[3] = inflater.inflate(R.layout.map_view, tabContentParent, false);
        tabContents[3].setVisibility(View.INVISIBLE);
        tabContentParent.addView(tabContents[3]);
        Packet.mapViewController = new MapViewController(tabContents[3], this);

        // Debugging
        Packet.generateTestPackets();

    }

    public void onTabClick(View v) {
        if (getCurrentFocus() != null) {
            InputMethodManager imm = (InputMethodManager)getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.hideSoftInputFromWindow(getCurrentFocus().getWindowToken(), 0);
        }
        for (int i = 0; i < numTabs; i++) {
            if (tabButtons[i].equals(v)) {
                if (i != activeTabIndex) {
                    tabHighlights[activeTabIndex].setVisibility(View.INVISIBLE);
                    tabHighlights[i].setVisibility(View.VISIBLE);
                    if (tabContents[activeTabIndex] != null) {
                        tabContents[activeTabIndex].setVisibility(View.INVISIBLE);
                    }
                    if (tabContents[i] != null) {
                        tabContents[i].setVisibility(View.VISIBLE);
                    }
                    activeTabIndex = i;
                    break;
                }
            }
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        // For determining whether audio jack / microphone is plugged into phone.
        IntentFilter filter = new IntentFilter(Intent.ACTION_HEADSET_PLUG);
        getApplicationContext().registerReceiver(mReceiver, filter);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        getApplicationContext().unregisterReceiver(mReceiver);
        AudioSerial.singleton.stopRX();
        sosViewController.stop();
    }

    //http://stackoverflow.com/questions/14708636/detect-whether-headset-has-microphone
    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            AudioSerial.singleton.ignoreNextMicReadsCount = 2;
            final String action = intent.getAction();
            if (Intent.ACTION_HEADSET_PLUG.equals(action)) {
                Log.d("HeadSetPlugInTest", "state: " + intent.getIntExtra("state", -1));
                Log.d("HeadSetPlugInTest", "microphone: " + intent.getIntExtra("microphone", -1));
                if (intent.getIntExtra("state", -1) != 1) {
                    Toast.makeText(MainActivity.genericContext, "No audio connection", Toast.LENGTH_SHORT).show();
                } else if (intent.getIntExtra("microphone", -1) == 1) {
                    Toast.makeText(MainActivity.genericContext, "Mic connected", Toast.LENGTH_SHORT).show();
                } else {
                    Toast.makeText(MainActivity.genericContext, "No Mic detected", Toast.LENGTH_SHORT).show();
                }
            }
        }
    };

    @Override
    public void onLocationChanged(Location location) {
        // TODO Auto-generated method stub
        currentLocation = location;
    }

    @Override
    public void onProviderDisabled(String provider) {
        // TODO Auto-generated method stub

    }

    @Override
    public void onProviderEnabled(String provider) {
        // TODO Auto-generated method stub

    }

    @Override
    public void onStatusChanged(String provider, int status, Bundle extras) {
        // TODO Auto-generated method stub

    }

}

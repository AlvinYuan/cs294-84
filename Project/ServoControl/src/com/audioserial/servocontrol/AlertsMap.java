package com.audioserial.servocontrol;

import com.google.android.gms.maps.CameraUpdateFactory;
import com.google.android.gms.maps.GoogleMap;
import com.google.android.gms.maps.SupportMapFragment;
import com.google.android.gms.maps.GoogleMap.InfoWindowAdapter;
import com.google.android.gms.maps.GoogleMap.OnMarkerClickListener;
import com.google.android.gms.maps.model.LatLng;
import com.google.android.gms.maps.model.Marker;
import com.google.android.gms.maps.model.MarkerOptions;

import android.os.Bundle;
import android.view.Menu;
import android.view.View;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;

public class AlertsMap extends FragmentActivity
        implements OnMarkerClickListener, InfoWindowAdapter {

    GoogleMap map;
    public static AlertsMap liveActivity = null;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_alerts_map);

        liveActivity = this;
        map = ((SupportMapFragment) getSupportFragmentManager().findFragmentById(R.id.map)).getMap();
        map.getUiSettings().setMyLocationButtonEnabled(true);
        map.setMyLocationEnabled(true);
        map.setInfoWindowAdapter(this);

        for (int i = 0; i < Packet.packetsReceived.size(); i++) {
            Packet p = Packet.packetsReceived.get(i);
            // Construct marker
            addPacketToMap(p);
        }
        /* Not perfectly centered for some reason, but very close. Good enough I guess. */
        LatLng location = new LatLng(MainActivity.currentLocation.getLatitude(), MainActivity.currentLocation.getLongitude());
        map.moveCamera(CameraUpdateFactory.newLatLngZoom(location, 15));
        // Zoom in, animating the camera.
        map.animateCamera(CameraUpdateFactory.zoomTo(15), 2000, null);

    }

    public void addPacketToMap(Packet p) {
        if (p.loc != null) {
            Marker marker = map.addMarker(
                    new MarkerOptions().position(new LatLng(p.loc.getLatitude(), p.loc.getLongitude())).title(p.packetType.readable).snippet(p.readableFormat()));
        }
    }
    @Override
    protected void onDestroy() {
        liveActivity = null;
        super.onDestroy();
    }
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.alerts_map, menu);
        return true;
    }

    @Override
    public View getInfoContents(Marker arg0) {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    // Use default InfoWindow frame
    public View getInfoWindow(Marker arg0) {
        return null;
    }

    @Override
    public boolean onMarkerClick(Marker arg0) {
        // TODO Auto-generated method stub
        return false;
    }

}

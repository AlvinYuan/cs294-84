package com.audioserial.servocontrol;

import com.google.android.gms.maps.CameraUpdateFactory;
import com.google.android.gms.maps.GoogleMap;
import com.google.android.gms.maps.SupportMapFragment;
import com.google.android.gms.maps.GoogleMap.InfoWindowAdapter;
import com.google.android.gms.maps.GoogleMap.OnMarkerClickListener;
import com.google.android.gms.maps.model.BitmapDescriptorFactory;
import com.google.android.gms.maps.model.LatLng;
import com.google.android.gms.maps.model.Marker;
import com.google.android.gms.maps.model.MarkerOptions;

import android.view.View;
import android.support.v4.app.FragmentActivity;

public class MapViewController implements OnMarkerClickListener, InfoWindowAdapter {

    GoogleMap map;

    public MapViewController(View map_view, FragmentActivity activity) {
        map = ((SupportMapFragment) activity.getSupportFragmentManager().findFragmentById(R.id.map)).getMap();
        map.getUiSettings().setMyLocationButtonEnabled(true);
        map.setMyLocationEnabled(true);
        map.setInfoWindowAdapter(this);

        for (int i = 0; i < Packet.packetsReceived.size(); i++) {
            Packet p = Packet.packetsReceived.get(i);
            // Construct marker
            addPacketToMap(p);
        }

        LatLng centerLocation;
        if (MainActivity.currentLocation != null) {
            centerLocation = new LatLng(MainActivity.currentLocation.getLatitude(), MainActivity.currentLocation.getLongitude());
        } else {
            centerLocation = new LatLng(37.8748,-122.2583);
        }

        map.moveCamera(CameraUpdateFactory.newLatLngZoom(centerLocation, 15));
        // Zoom in, animating the camera.
        map.animateCamera(CameraUpdateFactory.zoomTo(15), 2000, null);
    }

    public void addPacketToMap(Packet p) {
        if (p.loc != null) {
            float hue;
            switch (p.packetType) {
            case SOS:
                hue = BitmapDescriptorFactory.HUE_AZURE;
                break;
            case DANGER:
                switch (p.dangerLevel) {
                case PROCEED_WITH_CAUTION:
                    hue = BitmapDescriptorFactory.HUE_YELLOW;
                    break;
                case GENERAL_DANGER:
                    hue = BitmapDescriptorFactory.HUE_ORANGE;
                    break;
                case EVACUATE_THE_AREA:
                    hue = BitmapDescriptorFactory.HUE_RED;
                    break;
                default:
                    hue = BitmapDescriptorFactory.HUE_ORANGE;
                    break;
                }
                break;
            default:
                hue = BitmapDescriptorFactory.HUE_ROSE;
                break;
            }

            String title = "";
            if (p.dangerType != Packet.TypeOfDanger.NOT_SPECIFIED) {
                title += p.dangerType.readable;
                if (!p.customMessage.equals("")) {
                    title += ": ";
                }
            }
            title += p.customMessage;

            if (title.equals("")) {
                title = p.packetType.readable;
            }
            Marker marker = map.addMarker(
                    new MarkerOptions().
                    position(p.loc).
                    title(title).
                    icon(BitmapDescriptorFactory.defaultMarker(hue)));
        }
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

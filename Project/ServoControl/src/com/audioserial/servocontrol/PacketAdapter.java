package com.audioserial.servocontrol;

import java.util.HashMap;
import java.util.List;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.TextView;

public class PacketAdapter extends ArrayAdapter<Packet> implements OnClickListener {
    public LayoutInflater inflater;
    public HashMap<View, Integer> followedViewToPositionMap;

    public PacketAdapter(Context context, int textViewResourceId,
            List<Packet> objects) {
        super(context, textViewResourceId, objects);
        inflater = (LayoutInflater)context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        followedViewToPositionMap = new HashMap<View, Integer>();
    }

    public View getView(int position, View convertView, ViewGroup parent) {
        View v = convertView;
        if (convertView == null) {
            // Get the list element view
            v = inflater.inflate(R.layout.log_entry, null);
        }

        Packet p = (Packet) super.getItem(position);
        // Modify the list element view with information specific to p
        ((TextView) v.findViewById(R.id.MessageTextView)).setText(p.readableFormat());
        ((TextView) v.findViewById(R.id.TimestampTextView)).setText(p.timeStamp);
        return v;
    }

    @Override
    public void onClick(View view) {
        int position = followedViewToPositionMap.get(view);
        Packet p = getItem(position);
    }
}

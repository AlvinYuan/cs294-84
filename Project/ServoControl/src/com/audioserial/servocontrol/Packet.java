package com.audioserial.servocontrol;

import java.util.ArrayList;

import android.widget.Toast;

public class Packet {
    public static final char PACKET_DELIMITER = '\n';
    public static final char FIELD_DELIMITER = '|';

    public static ArrayList<Packet> packetsReceived = new ArrayList<Packet>();

    String customMessage;

    public static void retrievedNewPacket(Packet p) {
        Toast.makeText(MainActivity.genericContext, p.toString(), Toast.LENGTH_SHORT).show();
        packetsReceived.add(p);
    }

    public Packet(String packetString) {
        // Parse packetString for fields
        customMessage = packetString;
    }

    public String toString() {
        return customMessage + PACKET_DELIMITER;
    }
}

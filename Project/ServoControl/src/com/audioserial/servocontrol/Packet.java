package com.audioserial.servocontrol;

import java.util.ArrayList;

public class Packet {
    public static final char PACKET_DELIMITER = '\n';
    public static final char FIELD_DELIMITER = '|';

    public static ArrayList<Packet> packetsReceived = new ArrayList<Packet>();

    String customMessage;

    public Packet(String packetString) {
        // Parse packetString for fields
        customMessage = packetString;
    }

    public String toString() {
        return customMessage + PACKET_DELIMITER;
    }
}

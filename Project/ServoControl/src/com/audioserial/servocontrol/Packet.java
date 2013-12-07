package com.audioserial.servocontrol;

import java.util.ArrayList;

import android.widget.Toast;

public class Packet {
    public static final char PACKET_DELIMITER = '\n';
    public static final char FIELD_DELIMITER = '|';

    public static ArrayList<Packet> packetsReceived = new ArrayList<Packet>();

    enum PacketType {
        DANGER,
        SOS,
        NOT_SPECIFIED
    }
    enum TypeOfDanger {
        FIRE,
        CHEMICAL,
        UNSTABLE_SURROUNDINGS,
        NOT_SPECIFIED
    }

    enum LevelOfDanger {
        PROCEED_WITH_CAUTION,
        GENERAL_DANGER,
        EVACUATE_THE_AREA,
        NOT_SPECIFIED
    }
    PacketType packetType = PacketType.NOT_SPECIFIED;
    TypeOfDanger dangerType = TypeOfDanger.NOT_SPECIFIED;
    LevelOfDanger dangerLevel = LevelOfDanger.NOT_SPECIFIED;

    String customMessage;

    public static void retrievedNewPacket(Packet p) {
        Toast.makeText(MainActivity.genericContext, p.stringRepresentation(), Toast.LENGTH_SHORT).show();
        packetsReceived.add(p);
    }

    public Packet(PacketType packetType, TypeOfDanger dangerType, LevelOfDanger dangerLevel, String customMessage) {
        this.packetType = packetType;
        this.dangerType = dangerType;
        this.dangerLevel = dangerLevel;
        this.customMessage = customMessage;
    }
    public Packet(String packetString) {
        // Parse packetString for fields
        customMessage = packetString;
    }

    public String stringRepresentation() {
        String str = "";
        str += FIELD_DELIMITER + stringEncoding(packetType);
        if (packetType == PacketType.DANGER) {
            str += FIELD_DELIMITER + stringEncoding(dangerType);
            str += FIELD_DELIMITER + stringEncoding(dangerLevel);
        }
        str += FIELD_DELIMITER + customMessage;
        str += PACKET_DELIMITER;
        return str;
    }

    private static String stringEncoding(PacketType packetType) {
        switch (packetType) {
        case DANGER:
            return "D";
        case SOS:
            return "S";
        default:
            return "?";
        }
    }

    private static String stringEncoding(TypeOfDanger dangerType) {
        switch (dangerType) {
        case FIRE:
            return "F";
        case CHEMICAL:
            return "C";
        case UNSTABLE_SURROUNDINGS:
            return "U";
        default:
            return "";
        }
    }

    private static String stringEncoding(LevelOfDanger dangerLevel) {
        switch (dangerLevel) {
        case PROCEED_WITH_CAUTION:
            return "1";
        case GENERAL_DANGER:
            return "2";
        case EVACUATE_THE_AREA:
            return "3";
        default:
            return "?";
        }
    }
}

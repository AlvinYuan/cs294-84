package com.audioserial.servocontrol;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;

import android.location.Location;
import android.widget.Toast;

public class Packet {
    public static final char PACKET_DELIMITER = '\n';
    public static final char FIELD_DELIMITER = '|';
    public static final String FIELD_DELIMITER_REGEX = "\\|";

    public static ArrayList<Packet> packetsReceived = new ArrayList<Packet>();
    public static PacketAdapter packetAdapter = null; // Initialized in MainActivity.onCreate
    public static MapViewController mapViewController;

    enum PacketType {
        DANGER("D", "Danger Nearby"),
        SOS("S", "SOS"),
        NOT_SPECIFIED("?","");

        final String encoding;
        final String readable;
        PacketType(String encoding, String readable) {
            this.encoding = encoding;
            this.readable = readable;
        }
    }
    enum TypeOfDanger {
        FIRE("F", "Fire"),
        CHEMICAL("C", "Chemical"),
        UNSTABLE_SURROUNDINGS("U", "Structurally Unsound"),
        NOT_SPECIFIED("?", "");

        final String encoding;
        final String readable;
        TypeOfDanger(String encoding, String readable) {
            this.encoding = encoding;
            this.readable = readable;
        }
    }

    enum LevelOfDanger {
        PROCEED_WITH_CAUTION("1", "Proceed with Caution"),
        GENERAL_DANGER("2", "General Danger"),
        EVACUATE_THE_AREA("3", "Severe Danger, Evacuate the area"),
        NOT_SPECIFIED("?", "");

        final String encoding;
        final String readable;
        LevelOfDanger(String encoding, String readable) {
            this.encoding = encoding;
            this.readable = readable;
        }
    }

    // Fields
    PacketType packetType = PacketType.NOT_SPECIFIED;
    TypeOfDanger dangerType = TypeOfDanger.NOT_SPECIFIED;
    LevelOfDanger dangerLevel = LevelOfDanger.NOT_SPECIFIED;
    String customMessage = "";
    String timeStamp = new SimpleDateFormat("h:mm").format(Calendar.getInstance().getTime());
    Location loc = null;
    boolean locFromPacket; // if false, loc refers to location of user when they received the.packet.

    public static void retrievedNewPacket(Packet p) {
        Toast.makeText(MainActivity.genericContext, p.readableFormat(), Toast.LENGTH_SHORT).show();
        packetsReceived.add(0, p);
        packetAdapter.notifyDataSetChanged();
        if (mapViewController != null) {
            mapViewController.addPacketToMap(p);
        }
    }

    public Packet(PacketType packetType, TypeOfDanger dangerType, LevelOfDanger dangerLevel, String customMessage) {
        this.packetType = packetType;
        this.dangerType = dangerType;
        this.dangerLevel = dangerLevel;
        this.customMessage = customMessage;
    }
    public Packet(String packetString) {
        // Parse packetString for fields
        String[] fields = packetString.split(FIELD_DELIMITER_REGEX);

        // fields[0] should be an empty string

        if (fields.length > 1 && fields[1].length() > 0) {
            switch (fields[1].charAt(0)) {
            case 'D':
                packetType = PacketType.DANGER;
                break;
            case 'S':
                packetType = PacketType.SOS;
                break;
            }
        }
        switch (packetType) {
        case DANGER:
            if (fields.length > 2 && fields[2].length() > 0) {
                switch (fields[2].charAt(0)) {
                case '1':
                    dangerLevel = LevelOfDanger.PROCEED_WITH_CAUTION;
                    break;
                case '2':
                    dangerLevel = LevelOfDanger.GENERAL_DANGER;
                    break;
                case '3':
                    dangerLevel = LevelOfDanger.EVACUATE_THE_AREA;
                    break;
                }
            }
            if (fields.length > 3 && fields[3].length() > 0) {
                switch (fields[3].charAt(0)) {
                case 'F':
                    dangerType = TypeOfDanger.FIRE;
                    break;
                case 'C':
                    dangerType = TypeOfDanger.CHEMICAL;
                    break;
                case 'U':
                    dangerType = TypeOfDanger.UNSTABLE_SURROUNDINGS;
                    break;
                }
            }
            if (fields.length > 4) {
                customMessage = fields[4];
            }
            break;
        case SOS:
            // TODO: fill
        default:
            customMessage = packetString;
        }

        // TODO: encode loc in packets
        // For now, always set loc to user's current location
        loc = MainActivity.currentLocation;
        locFromPacket = false;
    }

    public String stringRepresentation() {
        String str = "";
        str += FIELD_DELIMITER + packetType.encoding;
        if (packetType == PacketType.DANGER) {
            str += FIELD_DELIMITER + dangerLevel.encoding;
            str += FIELD_DELIMITER + dangerType.encoding;
        }
        str += FIELD_DELIMITER + customMessage;
        str += PACKET_DELIMITER;
        return str;
    }

    public String readableFormat() {
        String str = "";
        if (packetType != PacketType.NOT_SPECIFIED) {
            str += packetType.readable + "--";
        }
        if (dangerLevel != LevelOfDanger.NOT_SPECIFIED) {
            str += dangerLevel.readable + "--";
        }
        if (dangerType != TypeOfDanger.NOT_SPECIFIED) {
            str += dangerType.readable + ": ";
        }
        str += customMessage;
        return str;
    }

    public static void generateTestPackets() {
        retrievedNewPacket(new Packet("|D|2|F|soda hall is on fire"));
        retrievedNewPacket(new Packet("|D|1|?|"));
        retrievedNewPacket(new Packet("|D|3|C|get out"));
        retrievedNewPacket(new Packet("|S|get out"));
    }

}

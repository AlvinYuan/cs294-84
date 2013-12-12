package com.audioserial.servocontrol;

import java.text.DecimalFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;

import com.google.android.gms.maps.model.LatLng;

import android.content.Context;
import android.os.Vibrator;
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
    LatLng loc = null;

    // Relevant for reading packets, but not for sending
    String timeStamp = new SimpleDateFormat("h:mm").format(Calendar.getInstance().getTime());
    boolean locFromPacket; // if false, loc refers to location of user when they received the packet.

    public static void retrievedNewPacket(Packet p, boolean silent) {
        if (p.packetType != PacketType.NOT_SPECIFIED) {
            if (!silent) {
                Toast.makeText(MainActivity.genericContext, "Received new message!", Toast.LENGTH_SHORT).show();
                Vibrator v = (Vibrator) MainActivity.genericContext.getSystemService(Context.VIBRATOR_SERVICE);
                v.vibrate(500);
            }

            packetsReceived.add(0, p);
            packetAdapter.notifyDataSetChanged();
            if (mapViewController != null) {
                mapViewController.addPacketToMap(p);
            }
        }
    }

    public Packet(PacketType packetType, TypeOfDanger dangerType, LevelOfDanger dangerLevel, String customMessage) {
        this.packetType = packetType;
        this.dangerType = dangerType;
        this.dangerLevel = dangerLevel;
        this.customMessage = customMessage;
        if (MainActivity.currentLocation != null) {
            loc = new LatLng(MainActivity.currentLocation.getLatitude(), MainActivity.currentLocation.getLongitude());
        }
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
            if (fields.length > 6 && fields[5].length() > 0 && fields[6].length() > 0) {
                try {
                    loc = new LatLng(Double.parseDouble(fields[5]), Double.parseDouble(fields[6]));
                    locFromPacket = true;
                } catch(Exception E) {
                    loc = null;
                }
            }
            break;
        case SOS:
            if (fields.length > 2) {
                customMessage = fields[2];
            }
            if (fields.length > 4 && fields[3].length() > 0 && fields[4].length() > 0) {
                try {
                    loc = new LatLng(Double.parseDouble(fields[3]), Double.parseDouble(fields[4]));
                    locFromPacket = true;
                } catch(Exception E) {
                    loc = null;
                }
            }
            break;
        default:
            customMessage = packetString;
        }

        if (loc == null && MainActivity.currentLocation != null) {
            loc = new LatLng(MainActivity.currentLocation.getLatitude(), MainActivity.currentLocation.getLongitude());
            locFromPacket = false;
        }
    }

    public String stringRepresentation() {
        String str = "";
        str += FIELD_DELIMITER + packetType.encoding;
        if (packetType == PacketType.DANGER) {
            str += FIELD_DELIMITER + dangerLevel.encoding;
            str += FIELD_DELIMITER + dangerType.encoding;
        }
        str += FIELD_DELIMITER + customMessage;
        str += FIELD_DELIMITER + (loc == null ? "" : new DecimalFormat("#.####").format(loc.latitude));
        str += FIELD_DELIMITER + (loc == null ? "" : new DecimalFormat("#.####").format(loc.longitude));
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
        double lat = MainActivity.currentLocation == null ? 37.8748 : MainActivity.currentLocation.getLatitude();
        double lng = MainActivity.currentLocation == null ? -122.2583 : MainActivity.currentLocation.getLongitude();
        Packet p = new Packet("|D|3|F|get out of the building now");
        p.loc = new LatLng(
                lat - .0005,
                lng - .0005);
        retrievedNewPacket(p, true);

        p = new Packet("|D|3|F|no one is hurt, but huge fire.");
        p.loc = new LatLng(
                lat - .00051,
                lng - .00049);
        retrievedNewPacket(p, true);

        p = new Packet("|D|3|F|");
        p.loc = new LatLng(
                lat - .00051,
                lng - .00048);
        retrievedNewPacket(p, true);

        p = new Packet("|D|2|?|");
        p.loc = new LatLng(
                lat + .0006,
                lng + .0007);
        retrievedNewPacket(p, true);

        p = new Packet("|D|1|U|avoid the cracked walls");
        p.loc = new LatLng(
                lat + .0009,
                lng + .0009);
        retrievedNewPacket(p, true);

        p = new Packet("|S|foot is stuck under rubble");
        p.loc = new LatLng(
                lat + .0009,
                lng + .0004);
        retrievedNewPacket(p, true);
        p = new Packet("|S|lost|37.8755|-122.2568");
        retrievedNewPacket(p, true);
        p = new Packet("|D|?|?|confusion|37.8756|-122.2569");
        retrievedNewPacket(p, true);
    }

}

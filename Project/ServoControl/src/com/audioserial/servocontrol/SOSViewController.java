package com.audioserial.servocontrol;

import android.app.Activity;
import android.os.Handler;
import android.view.View;
import android.widget.EditText;

public class SOSViewController {
    public static final int SOS_PERIODIC_SENDING = 5000;
    EditText details;
    Handler periodicSOShandler = new Handler();
    Runnable periodicSOSRunnable = new Runnable() {
        public void run() {
            periodicSOSRun();
        }
    };

    public SOSViewController(View dangerView, Activity activity) {
        details = (EditText) dangerView.findViewById(R.id.SosDetailsEditText);
        periodicSOShandler.postDelayed(periodicSOSRunnable, SOS_PERIODIC_SENDING);
    }

    public void stop() {
        periodicSOShandler.removeCallbacks(periodicSOSRunnable);
    }

    void periodicSOSRun() {
        String str = details.getText().toString();
        if (!str.equals("")) {
            AudioSerial.singleton.send(new Packet(Packet.PacketType.SOS,
                                                  Packet.TypeOfDanger.NOT_SPECIFIED,
                                                  Packet.LevelOfDanger.NOT_SPECIFIED,
                                                  str),
                                       true);
        }

        periodicSOShandler.postDelayed(periodicSOSRunnable, SOS_PERIODIC_SENDING);
    }

}

import processing.serial.*;
import java.awt.Robot;
import java.awt.event.KeyEvent;
 
Serial myPort;    // The serial port

Robot robot;
void setup() {  
  myPort = new Serial(this, Serial.list()[0], 9600); 
  myPort.bufferUntil('\n');
  try {
    robot = new Robot();
  } catch (Exception e) {
    println("No robot created");
    return;
  }
  println("Ready to Go!");
} 

void draw() {
  // Must have implementation, even if blank?
}

void serialEvent(Serial p) {
  String in = p.readStringUntil('\n');
  in = in.substring(0, in.length()-2); //not sure why -2. Perhaps something with \n being converted to 2 characters.
  keyEvent(in);
}

void keyEvent(String s) {
  if (s.equals("NITRO")) {
    robot.keyPress(KeyEvent.VK_N);
  } else if (s.equals("BACKVIEW")) {
    robot.keyPress(KeyEvent.VK_B);
  } else if (s.equals("DRIFT")) {
    robot.keyPress(KeyEvent.VK_V);
  } else if (s.equals("ITEM")) {
    robot.keyPress(KeyEvent.VK_SPACE);
  } else if (s.equals("NITRO_R")) {
    robot.keyRelease(KeyEvent.VK_N);
  } else if (s.equals("BACKVIEW_R")) {
    robot.keyRelease(KeyEvent.VK_B);
  } else if (s.equals("DRIFT_R")) {
    robot.keyRelease(KeyEvent.VK_V);
  } else if (s.equals("ITEM_R")) {
    robot.keyRelease(KeyEvent.VK_SPACE);
  } else if (s.equals("LEFT")) {
    robot.keyPress(KeyEvent.VK_LEFT);
  } else if (s.equals("LEFT_R")) {
    robot.keyRelease(KeyEvent.VK_LEFT);
  } else if (s.equals("RIGHT")) {
    robot.keyPress(KeyEvent.VK_RIGHT);
  } else if (s.equals("RIGHT_R")) {
    robot.keyRelease(KeyEvent.VK_RIGHT);
  } else if (s.equals("ACCELERATE")) {
    robot.keyPress(KeyEvent.VK_UP);
  } else if (s.equals("ACCELERATE_R")) {
    robot.keyRelease(KeyEvent.VK_UP);
  } else if (s.equals("BRAKE")) {
    robot.keyPress(KeyEvent.VK_DOWN);
  } else if (s.equals("BRAKE_R")) {
    robot.keyRelease(KeyEvent.VK_DOWN);
  } else {
    println("bad string");
    println(s);
    println(s.length());
  }
}


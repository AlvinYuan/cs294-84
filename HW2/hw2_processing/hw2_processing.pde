import processing.serial.*; 
 
Serial myPort;    // The serial port
PFont largeFont = createFont("Courier New Bold", 24, false);
PFont smallFont = createFont("Courier New", 12, false);
String inputString = "";
void setup() { 
  size(600,200); 
//  println(Serial.list());
  myPort = new Serial(this, Serial.list()[0], 9600); 
  myPort.bufferUntil('\n'); 
} 
 
void draw() {
  background(255);
  fill(0,0,0);
  textFont(largeFont);
  text("A    E    I      O      U", 10,47); 
  text(" BCD  FGH  JKLMN  PQRST  VWXYZ _<.,?!\n1234 1234 123456 123456 123456 123456\n", 10,50); 
  textFont(smallFont);
  text("_ = {Space}    < = {Backspace}", 10, 120);
  textFont(largeFont);
  text(inputString, 10, 150);

  fill(200,200,200);
  text("_", 10 + textWidth(inputString), 150);
} 
 
void serialEvent(Serial p) {
  char input = p.readStringUntil('\n').charAt(0);
  if (input == 8) {
    if (inputString.length() > 0) {     
      inputString = inputString.substring(0,inputString.length() - 1);
    }
  } else {
    inputString = inputString + input;
  }
  println(inputString);
}


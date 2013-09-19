const int numButtons = 6;
const int buttonPins[numButtons] = {2, 3, 4, 5, 6, 7};
// If Processing handles display,\x08 (backspace) works. If outputting to Serial Monitor, backspace won't work.
const char buttonsToChar[6][6] = {{'A','B','C','D','E','F'},
                                  {'E','F','G','H','I','J'},
                                  {'I','J','K','L','M','N'},
                                  {'O','P','Q','R','S','T'},
                                  {'U','V','W','X','Y','Z'},
                                  {' ','\x08','.',',','?','!'}};
/*
 * Text Entry State
 */
enum textEntryState {
  ZeroButtonsPressed,
  OneButtonPressed
};
textEntryState state = ZeroButtonsPressed;
int buttonPressed[2] = {-1, -1};

/*
 * Debouncing State
 * Based on Debounce tutorial.
 */
int buttonState[numButtons];
int previousReading[numButtons] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
long lastDebounceTime[numButtons] = {0, 0, 0, 0, 0, 0};

const long debounceDelay = 50; // debounce time threshold

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < numButtons; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  pinMode(13, OUTPUT);
  
//  Serial.println("ABCD EFGH IJKLMN OPQRST UVWXYZ _<.,?!\n1234 1234 123456 123456 123456 123456\n");
}

void loop() {
  for (int i = 0; i < numButtons; i++) {    
    int reading = digitalRead(buttonPins[i]);
    
    if (reading != previousReading[i]) {
      // Potential state change detected. Set time marker.
      lastDebounceTime[i] = millis();
    }
    
    if (millis() - lastDebounceTime[i] > debounceDelay) {
    // Debounce time threshold passed
      if (reading != buttonState[i]) {
        // Actual State change detected.
        buttonState[i] = reading;
        if (reading == LOW) {
          // Button pressed.
          updateTextEntryState(i);
        }
      }
      
    }

    // Update previous reading.
    previousReading[i] = reading;
  }
}

void updateTextEntryState(int i) {
    switch (state) {
    case ZeroButtonsPressed:
      buttonPressed[0] = i;
      state = OneButtonPressed;
      break;
    
    case OneButtonPressed:
      buttonPressed[1] = i;
      state = ZeroButtonsPressed;
      Serial.println(buttonsToChar[buttonPressed[0]][buttonPressed[1]]);
      break;
  }
}


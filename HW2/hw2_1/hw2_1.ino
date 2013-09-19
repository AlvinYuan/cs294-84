const int numButtons = 9;
const int buttonPins[numButtons] = {2, 3, 4, 5, 6, 7, 8, 9, 10};
const char buttonsToChar[3][3][3] = {{{'A','B','C'},{'D','E','F'},{'G','H','I'}},
                                     {{'J','K','L'},{'M','N','O'},{'P','Q','R'}},
                                     {{'S','T','U'},{'V','W','X'},{'Y','Z',' '}}};
/*
 * Text Entry State
 */
enum textEntryState {
  ZeroButtonsPressed,
  OneButtonPressed,
  TwoButtonsPressed,
};
textEntryState state = ZeroButtonsPressed;
int buttonPressed[3] = {-1, -1, -1};

/*
 * Debouncing State
 * Based on Debounce tutorial.
 */
int buttonState[numButtons];
int previousReading[numButtons] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};
long lastDebounceTime[numButtons] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

const long debounceDelay = 50; // debounce time threshold

void setup() {
  Serial.begin(9600);
  for (int i = 0; i < numButtons; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  pinMode(13, OUTPUT);
  
//  Serial.println("Hello World.");
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
//          Serial.println(i);
        }
      }
      
    }

    // Update previous reading.
    previousReading[i] = reading;
  }
}

void updateTextEntryState(int i) {
    if (0 <= i && i <=2) {
      buttonPressed[0] = i;
    }
    if (3 <= i && i <=5) {
      buttonPressed[1] = i - 3;
    }
    if (6 <= i && i <= 8) {
      buttonPressed[2] = i - 6;
    }
    int numButtonsPressed = 0;
    for (int j = 0; j < 3; j++) {
      if (buttonPressed[j] != -1) {
        numButtonsPressed++;
      }
    }

    switch (state) {
    case ZeroButtonsPressed:
      if (numButtonsPressed == 1) {
        state = OneButtonPressed;
      }
      break;
    
    case OneButtonPressed:
      if (numButtonsPressed == 2) {
        state = TwoButtonsPressed;
      }
      break;
      
    case TwoButtonsPressed:
      if (numButtonsPressed == 3) {
        state = ZeroButtonsPressed;
        Serial.print(buttonsToChar[buttonPressed[0]][buttonPressed[1]][buttonPressed[2]]);
        for (int j = 0; j < 3; j++) {
          buttonPressed[j] = -1;
        }
      }
      break;
  }
}


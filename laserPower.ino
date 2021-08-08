#include <Wire.h>
#include <SerLCD.h>
#include <SparkFun_Qwiic_Keypad_Arduino_Library.h>

const byte RXLED = 17;  // The RX LED has a defined Arduino pin
const byte analogPin = 5;   // laser connected to analog pin 3
const byte keypadInterruptPin = 7;
const byte fireButtonPin = 4;

volatile bool interruptOccurred = false;

const int keypadBufferLength = 20;
char keypadBuffer[keypadBufferLength];
int keypadBufferPos = 0;
const char PIN[] = "1234";

volatile bool powerOn = false;
volatile bool fireState = false;
volatile bool fireStateChanged = false;
int powerLevel = 0;
bool pinEntered = false;

KEYPAD keypad;
SerLCD lcd; // Initialize the library with default I2C address 0x72

void KeypadEvent()
{
  interruptOccurred = true;
}

void setup() {
  pinMode(RXLED, OUTPUT);  // sets the pin as output
  pinMode(analogPin, OUTPUT);
  pinMode(fireButtonPin, INPUT_PULLUP);
  pinMode(keypadInterruptPin, INPUT); // Qwiic Keypad holds INT pin HIGH @ 3.3V, then LOW when fired.
  // Note, this means we do not want INPUT_PULLUP.
  
  // ASAP
  setLaserPower(false, 0);

  attachInterrupt(digitalPinToInterrupt(keypadInterruptPin), KeypadEvent, FALLING);
  Wire.begin();
  Wire.setClock(400000); //Optional - set I2C SCL to High Speed Mode of 400kHz
  lcd.begin(Wire);
  lcd.setBacklight(0, 0, 255);
  lcd.clear(); //Clear the display - this moves the cursor to home position as well
  lcd.println("Enter PIN");
  memset(keypadBuffer, 0, keypadBufferLength);

  if (keypad.begin() == false)   // Note, using begin() like this will use default I2C address, 0x4B.
    // You can pass begin() a different address like so: keypad1.begin(Wire, 0x4A).
  {
    Serial.println("Keypad does not appear to be connected. Please check wiring. Freezing...");
    lcd.println("Keypad missing");
    while (1);
  }
}

void setLaserPower(bool on, int level)
{
  Serial.println(on ? "POWER ON" : "POWER OFF");
  Serial.println(level);
  lcd.clear();
  lcd.println(on ? "POWER ON" : "POWER OFF");
  lcd.println(level);
  digitalWrite(RXLED, on ? 0 : 1); // inverted
  if (!on) {
    digitalWrite(analogPin, false);
  } else {
    analogWrite(analogPin, level);
  }
  powerOn = on;
  powerLevel = level;
}

void readCharToBuffer(char c)
{
  if (keypadBufferPos == (keypadBufferLength - 2)) {
    // move down;
    for (int i = keypadBufferPos; i >= 1; i--) {
      keypadBuffer[i - 1] = keypadBuffer[i];
    }
  }
  keypadBuffer[keypadBufferPos] = c;
  keypadBuffer[keypadBufferPos + 1] = 0;
  if (keypadBufferPos < (keypadBufferLength - 1)) {
    keypadBufferPos++;
  }
}

bool checkPIN(char digit)
{
  if (pinEntered) {
    return true;
  }

  readCharToBuffer(digit);
  if (strcmp(keypadBuffer, PIN) == 0) {
    keypadBufferPos = 0;
    memset(keypadBuffer, 0, keypadBufferLength);
    return true;
  }

  return false;
}

void loop()
{
  if (pinEntered) {
    bool newState = digitalRead(fireButtonPin) == LOW; // active low

    if (newState != fireState) {
      fireStateChanged = true;
      fireState = newState;
      setLaserPower(newState, powerLevel);
    }
  }

  char button = char(keypad.getButton());

  if (interruptOccurred || (button > 0))
  {
    interruptOccurred = false;

    keypad.updateFIFO();  // necessary for keypad to pull button from stack to readable register

    if (button == -1)
    {
      Serial.println("No keypad detected");    
    }
    else if (button != 0)
    {    
      if (!pinEntered) {
        if (button == '#') {
          lcd.clear();
          keypadBufferPos = 0;
          memset(keypadBuffer, 0, keypadBufferLength);
        } else {
          lcd.write('*');
          if (checkPIN(button)) {
            pinEntered = true;
            lcd.clear();
            lcd.println("Laser Enabled");
          }
        }

      } else if (pinEntered && (keypadBufferPos == 0) && (powerLevel < 255) && (button == '#')) {
        setLaserPower(powerOn, powerLevel + 1);
    
      } else if (pinEntered && (keypadBufferPos == 0) && (powerLevel > 0) && (button == '*')) {
        setLaserPower(powerOn, powerLevel - 1);

      } else if (button == '#') {
        // try to adjust power
        int testPower = atoi(keypadBuffer);
        if (testPower >= 0 && testPower <= 255) {
          setLaserPower(powerOn, testPower);
        }
        keypadBufferPos = 0;
        memset(keypadBuffer, 0, keypadBufferLength);

      } else {
        readCharToBuffer(button);
        lcd.write(button);

      }
    }
  }
}

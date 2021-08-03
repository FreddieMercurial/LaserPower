#include <SparkFun_Qwiic_Twist_Arduino_Library.h>

const byte RXLED = 17;  // The RX LED has a defined Arduino pin
const byte analogPin = 5;   // laser connected to analog pin 3
const byte twistInterruptPin = 7; //Can be any GPIO
TWIST twist; //Create instance of this object
bool powerOn = false;
byte powerLevel = 0;
volatile bool interruptOccurred = false;
bool lastPressed = false;

void TwistEvent()
{
  interruptOccurred = true;;
}

void setup() {
  pinMode(RXLED, OUTPUT);  // sets the pin as output
  pinMode(analogPin, OUTPUT);
  pinMode(twistInterruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(twistInterruptPin), TwistEvent, FALLING);
  Wire.begin();
  if (twist.begin() == false)
  {
    Serial.println("Twist does not appear to be connected. Please check wiring. Freezing...");
    while (1)
    {
        digitalWrite(RXLED, powerOn);        
        delay(500);
        powerOn = !powerOn;
    }
  }

  // ASAP
  setLaserPower(false, 0);

  //Optional: You can modify the time between when the user has stopped turning and when interrupt is raised
  twist.setIntTimeout(100); //Set twist timeout to 500ms before interrupt assertion 
  twist.setLimit(255);
  twist.setColor(0, 0, 0); //Set Red and Blue LED brightnesses to half of max.
  twist.connectRed(0);
  twist.connectGreen(0);
  twist.connectBlue(10); //Blue LED will go up 10 in brightness with each encoder tick
}

void setLaserPower(bool on, byte level)
{
  Serial.println(on ? "POWER ON" : "POWER OFF");
  Serial.println(level);
  digitalWrite(RXLED, on ? 0 : 1); // inverted
  if (!on) {
    digitalWrite(analogPin, false);
  } else {
    TXLED1;
    analogWrite(analogPin, level);
  }
  powerOn = on;
  powerLevel = level;
}


void loop()
{
 if (interruptOccurred || twist.isMoved() || (twist.isPressed() != lastPressed))
  {
    interruptOccurred = false;
    bool pressed = twist.isPressed();
    setLaserPower(pressed, twist.getCount());
    lastPressed = pressed;
    twist.clearInterrupts(); //Clear any interrupt bits
  }
}

#include <ESP32Servo.h>
// variable declarations
static const int servoPin = 15;
static const int buttonPin = 4;
Servo servo1;

const int pinLDR =34;
float ldrValue;
const int pinLED = 23;
float ledValue;


void setup() {
  Serial.begin(115200);
  servo1.attach(servoPin);
  pinMode(buttonPin, INPUT);
}


void loop() {
  int buttonState;
  // manual feeding
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW) {
    // servo movement
    for(int posDegrees = 0; posDegrees <= 180; posDegrees++) {
    servo1.write(posDegrees);
    Serial.println(posDegrees);
    delay(20);
    }
    
    for(int posDegrees = 180; posDegrees >= 0; posDegrees--) {
    servo1.write(posDegrees);
    Serial.println(posDegrees);
    delay(20);
    }
  }
  // food detection
  ldrValue = analogRead(pinLDR); //read the status of the LDR value
  Serial.println(ldrValue);
  delay(2000);
  
  if (ldrValue <= 930){
    analogWrite(pinLED, HIGH);
    Serial.println("Feeder is Empty");
  }
  else{
    analogWrite(pinLED, LOW);
  }
}


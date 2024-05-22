#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>

// variable declarations
static const int servoPin = 15;
static const int buttonPin = 4;
Servo servo1;

const int pinLED = 23;
const int pinLDR =34;
int lcdColumns = 16;
int lcdRows = 2;

LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); 

void setup() {
  Serial.begin(9600);
  servo1.attach(servoPin);
  pinMode(buttonPin, INPUT);
  lcd.init();
  lcd.backlight();
}

void loop() {
  // food detection
  if (analogRead(pinLDR) <= 3000){
    analogWrite(pinLED, HIGH);
    Serial.print(analogRead(pinLDR));
    Serial.println(" Light");
    delay(200);
    // lcd indication
    lcd.setCursor(0, 0);
    lcd.print("Refilling");
    lcd.setCursor(0,1);
    lcd.print("Needed");
  }
  else{
    analogWrite(pinLED, LOW);
    Serial.print(analogRead(pinLDR));
    Serial.println(" Dark");
    delay(200);
    // clears the display to print new message
    lcd.clear();
  } 

  // manual feeding
  if (digitalRead(buttonPin) == HIGH) {
    for(int posDegrees = 0; posDegrees <= 180; posDegrees++) {
      servo1.write(posDegrees);
      delay(20);
    }
    for(int posDegrees = 180; posDegrees >= 0; posDegrees--) {
      servo1.write(posDegrees);
      delay(20);
    }
  }
}


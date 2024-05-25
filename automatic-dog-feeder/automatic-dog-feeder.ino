// Load libraries
#include <WiFi.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>

Servo servo1;  // create servo object to control a servo
static const int servoPin = 15;

static const int buttonPin = 4;
const int pinLED = 23;
const int pinLDR =34;
String ldrState = ""; // Auxiliar variables to store the current LDR state

int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); 

// network credentials
// const char* ssid     = "BHS_MC";
// const char* password = "20307ESPdev";
const char* ssid     = "Your new Wi-Fi";
const char* password = "Braidboy_99";

// Set web server port number to 80
WiFiServer server(80);
// Variable to store the HTTP request
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void servo(){
  Serial.println("Servo on");
  for(int posDegrees = 0; posDegrees <= 180; posDegrees++) {
    servo1.write(posDegrees);
    delay(20);
  }
  for(int posDegrees = 180; posDegrees >= 0; posDegrees--) {
    servo1.write(posDegrees);
    delay(20);
  }
}

void setup() {
  Serial.begin(9600);
  servo1.attach(servoPin);  // attaches the servo on the servoPin to the servo object
  pinMode(buttonPin, INPUT);
  lcd.init();
  lcd.backlight();

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());
  server.begin();
}

void loop(){
  // manual feeding
  if (digitalRead(buttonPin) == HIGH) {
    servo();
  }
  // food detection
  if (analogRead(pinLDR) <= 3000){
    analogWrite(pinLED, HIGH);
    Serial.print(analogRead(pinLDR));
    Serial.println(" Light");
    delay(200);
    ldrState = "on";
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
    ldrState = "off";
    // clears the display to print new message
    lcd.clear();
  }

  WiFiClient client = server.available();   // Listen for incoming clients
  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // rotates the servo
            if (header.indexOf("GET /on") >= 0) {
              servo();
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the page
            client.println("<style> html{ font-family: Helvetica; background-color:#0F0F0F; display: inline-block; margin: 0px auto; color:#FFF; text-align: center; }");
            client.println(".button{ background-color: #04AA6D; border: none; color: white; padding: 20px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin: 4px 2px; cursor: pointer; border-radius: 8px; }");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;} </style></head>");
            // Web Page Heading
            client.println("<body><h1>Dog Feeder</h1>");          
            client.println("<p>Remotely turn the feeder on</p>");
            // Button
            client.println("<p><a href=\"/on\"><button class=\"button\">FEED DOG</button></a></p>");


            // If the LDR recieves brightness, it displays a message
            if (ldrState == "on"){ 
              client.println("<p>Dog feeder is empty!</p>");
            } 
            else{
              client.println("<p>Dog feeder is full</p>");
            }

            client.println("</body></html>");
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
#include <WiFi.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>
#include <vector>

Servo servo1;  // create servo object to control a servo
static const int servoPin = 15;
static const int buttonPin = 4;
const int pinLED = 23;
const int pinLDR = 34;
String ldrState = ""; // Auxiliary variables to store the current LDR state
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// network credentials
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
// Define timeout time in millisec
const long timeoutTime = 2000;

// Feeding interval in milliseconds
unsigned long feedingInterval = 240000; // Default to 4 minutes for testing
unsigned long lastFeedingTime = 0;

void servo() {
  Serial.println("Servo on");
  lcd.setCursor(0, 0);
  lcd.print("Feeder is ON  ");
  for (int posDegrees = 0; posDegrees <= 180; posDegrees++) {
    servo1.write(posDegrees);
    delay(20);
  }
  for (int posDegrees = 180; posDegrees >= 0; posDegrees--) {
    servo1.write(posDegrees);
    delay(20);
  }
  lcd.setCursor(0, 0);
  lcd.print("Feeder is OFF ");
}

String getCurrentTime() {
  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  char timeString[64];
  strftime(timeString, sizeof(timeString), "%I:%M:%S (%B %d, %Y)", &timeinfo);
  return String(timeString);
}

String getNextFeedingTime() {
  time_t nextFeeding = time(nullptr) + (feedingInterval / 1000);
  struct tm nextFeedingInfo;
  localtime_r(&nextFeeding, &nextFeedingInfo);
  char nextFeedingString[64];
  strftime(nextFeedingString, sizeof(nextFeedingString), "%I:%M:%S", &nextFeedingInfo);
  return String(nextFeedingString);
}

String processor(const String& var) {
  if (var == "CURRENT_TIME") {
    return getCurrentTime();
  } else if (var == "CURRENT_INTERVAL") {
    return String(feedingInterval / 60000);
  } else if (var == "NEXT_FEEDING") {
    return getNextFeedingTime();
  }
  return String();
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
  // Print local IP address
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // start web server
  server.begin();

  // Initialize the time
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  // Wait for time to be set
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.println("Waiting for time");
    delay(1000);
  }
  Serial.println("Time synchronized");

  // Initialize last feeding time
  lastFeedingTime = millis();
}

void loop() {
  // manual feeding
  if (digitalRead(buttonPin) == HIGH) {
    servo();
  }
  // automatic feeding based on interval
  if (millis() - lastFeedingTime >= feedingInterval) {
    servo();
    lastFeedingTime = millis();
  }
  // food detection
  if (analogRead(pinLDR) <= 3000) {
    digitalWrite(pinLED, HIGH);
    delay(200);
    ldrState = "on";
    // lcd indication
    lcd.setCursor(0, 0);
    lcd.print("Refilling      ");
    lcd.setCursor(0, 1);
    lcd.print("Needed         ");
  } else {
    digitalWrite(pinLED, LOW);
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

            // rotates the servo if the feeder is not empty
            if (header.indexOf("GET /on") >= 0) {
              if (ldrState == "off") {
                servo();
                lastFeedingTime = millis(); // Reset last feeding time
              } else {
                client.println("<script>alert('Cannot feed. The feeder is empty!');</script>");
              }
            }

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the page using Bootstrap
            client.println("<link href=\"https://maxcdn.bootstrapcdn.com/bootstrap/4.5.2/css/bootstrap.min.css\" rel=\"stylesheet\">");
            client.println("<style> body{ background-color:#343a40; color:#fff; } .container { margin-top: 50px; } .card { background-color: #212529; color: #fff; border: none; } .btn-primary { background-color: #04AA6D; border: none; } .btn-primary:hover { background-color: #029a5d; } .interval-card { background-color: #495057; padding: 20px; border-radius: 10px; } </style>");
            client.println("<title>Dog Feeder</title>");
            client.println("<script>function confirmIntervalChange() {");
            client.println("return confirm('Are you sure you want to change the feeding interval?');");
            client.println("}</script></head>");

            // Web Page Heading
            client.println("<body><div class='container'><div class='card'><div class='card-body'><h1 class='card-title text-center'>Dog Feeder</h1>");
            // If the LDR receives brightness, it displays a message
            if (ldrState == "on") { 
              client.println("<p class='text-center text-danger'>Dog feeder is empty!</p>");
            } else {
              client.println("<p class='text-center text-success'>Dog feeder is full</p>");
            }
            client.println("<p class='text-center'>Remotely turn the feeder on</p>");
            // Button
            client.println("<div class='text-center'><a href=\"/on\" class='btn btn-primary'>Feed Dog</a></div>");

            // Displaying current time, interval, and next feeding time
            client.println("<div class='card mt-4'><div class='card-body'><h3 class='card-title'>" + getCurrentTime() + "</h3>");
            client.println("<p>Current Feeding Interval: " + processor("CURRENT_INTERVAL") + " minutes</p>");
            client.println("<p>Next Feeding Time: " + processor("NEXT_FEEDING") + "</p>");
            client.println("<div class='interval-card mt-4'><p>Set Feeding Interval (in minutes, 1 to 1440):</p>");
            client.println("<form action=\"/setinterval\" method=\"POST\" onsubmit=\"return confirmIntervalChange();\"><label for=\"interval\">Interval:</label><input type=\"number\" id=\"interval\" name=\"interval\" min=\"1\" max=\"1440\" required><br><br><input type=\"submit\" value=\"Set Interval\" class='btn btn-primary'></form></div></div></div></div></div>");
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

  // Check if a new interval has been set via POST request
  if (header.indexOf("POST /setinterval") >= 0) {
    int intervalIndex = header.indexOf("interval=");
    if (intervalIndex >= 0) {
      String intervalString = header.substring(intervalIndex + 9);
      intervalString = intervalString.substring(0, intervalString.indexOf(" "));
      // Read the LDR value to determine if the feeder is empty
      int ldrValue = analogRead(pinLDR);
      if (ldrValue > 3000) { // Only update if the feeder is not empty
        feedingInterval = intervalString.toInt() * 60000; // Convert to milliseconds
        Serial.println("Feeding interval updated to " + String(feedingInterval / 60000) + " minutes");
      } else {
        Serial.println("Cannot update interval while feeder is empty");
      }
    }
    header = ""; // Clear the header variable
  }
}

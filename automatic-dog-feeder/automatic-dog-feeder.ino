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
String ldrState = ""; // Auxiliary variable to store the current LDR state
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// network credentials
const char* ssid     = "BHS_MC";
const char* password = "20307ESPdev";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;
String body;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in millisec
const long timeoutTime = 2000;

// Feeding interval in milliseconds
unsigned long feedingInterval = 240000; // Default to 4 minutes for testing
unsigned long lastFeedingTimeMillis = 0;
time_t lastFeedingTimeRTC = 0;

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
  time_t nextFeeding = lastFeedingTimeRTC + (feedingInterval / 1000);
  struct tm nextFeedingInfo;
  localtime_r(&nextFeeding, &nextFeedingInfo);
  char nextFeedingString[64];
  strftime(nextFeedingString, sizeof(nextFeedingString), "%I:%M:%S", &nextFeedingInfo);
  return String(nextFeedingString);
}

String processor(const String& var) {
  if (var == "CURRENT_TIME") {
    return getCurrentTime();
  } 
  else if (var == "CURRENT_INTERVAL") {
    return String(feedingInterval / 60000);
  } 
  else if (var == "NEXT_FEEDING") {
    return getNextFeedingTime();
  }
  return String();
}

void setup() {
  Serial.begin(9600);

  servo1.attach(servoPin);
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
  Serial.println("WiFi connected");
  // Print local IP address
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
  lastFeedingTimeMillis = millis();
  lastFeedingTimeRTC = time(nullptr);
}

void handleClient(WiFiClient client) {
  currentTime = millis();
  previousTime = currentTime;
  Serial.println("New Client.");
  String currentLine = "";                
  header = "";
  body = "";
  bool isPost = false;

  while (client.connected() && currentTime - previousTime <= timeoutTime) {  
    currentTime = millis();
    if (client.available()) {             
      char c = client.read();             
      Serial.write(c);                    
      header += c;
      if (c == '\n' && currentLine.length() == 0) {  // Header end
        if (header.indexOf("POST /setinterval") >= 0) {
          isPost = true;
        }
      }
      if (c == '\n' && currentLine.length() == 0 && isPost) {
        while (client.available()) {
          c = client.read();
          body += c;
        }
        break;
      }
      if (c == '\n') {
        currentLine = "";
      } else if (c != '\r') {
        currentLine += c;
      }
    }
  }

  if (isPost) {
    int intervalIndex = body.indexOf("interval=");
    if (intervalIndex >= 0) {
      String intervalString = body.substring(intervalIndex + 9);
      intervalString = intervalString.substring(0, intervalString.indexOf("&"));
      int ldrValue = analogRead(pinLDR);
      if (ldrValue > 3000) { // Only update if the feeder is not empty
        feedingInterval = intervalString.toInt() * 60000; // Convert to milliseconds
        Serial.println("Feeding interval updated to " + String(feedingInterval / 60000) + " minutes");
      } else {
        Serial.println("Cannot update interval while feeder is empty");
      }
    }
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();

  if (header.indexOf("GET /on") >= 0) {
    if (ldrState == "off") {
      servo();
      lastFeedingTimeMillis = millis();
      lastFeedingTimeRTC = time(nullptr); // Update lastFeedingTimeRTC to current time
    } else {
      client.println("<script>alert('Cannot feed. The feeder is empty!');</script>");
    }
  }

  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<link rel=\"icon\" href=\"data:,\">");
  client.println("<link href=\"https://maxcdn.bootstrapcdn.com/bootstrap/4.5.2/css/bootstrap.min.css\" rel=\"stylesheet\">");
  client.println("<style> body{ background-color:#343a40; color:#fff; } .container { margin-top: 50px; } .card { background-color: #212529; color: #fff; border: none; } .btn-primary { background-color: #04AA6D; border: none; } .btn-primary:hover { background-color: #029a5d; } .interval-card { background-color: #495057; padding: 20px; border-radius: 10px; } </style>");
  client.println("<title>Dog Feeder</title>");
  client.println("<script>function confirmIntervalChange() {");
  client.println("return confirm('Are you sure you want to change the feeding interval?');");
  client.println("}</script></head>");

  client.println("<body><div class='container'><div class='card'><div class='card-body'><h1 class='card-title text-center'>Dog Feeder</h1>");
  if (ldrState == "on") { 
    client.println("<p class='text-center text-danger'>Dog feeder is empty!</p>");
  } else {
    client.println("<p class='text-center text-success'>Dog feeder is full</p>");
  }
  client.println("<p class='text-center'>Remotely turn the feeder on</p>");
  client.println("<div class='text-center'><a href=\"/on\" class='btn btn-primary'>Feed Dog</a></div>");

  client.println("<div class='card mt-4'><div class='card-body'><h3 class='card-title'>" + getCurrentTime() + "</h3>");
  client.println("<p>Current Feeding Interval: " + processor("CURRENT_INTERVAL") + " minutes</p>");
  client.println("<p>Next Feeding Time: " + processor("NEXT_FEEDING") + "</p>");
  client.println("<div class='interval-card mt-4'><p>Set Feeding Interval (in minutes, 1 to 1440):</p>");
  client.println("<form action=\"/setinterval\" method=\"POST\" onsubmit=\"return confirmIntervalChange();\"><label for=\"interval\">Interval:</label><input type=\"number\" id=\"interval\" name=\"interval\" min=\"1\" max=\"1440\" required><br><br><input type=\"submit\" value=\"Set Interval\" class='btn btn-primary'></form></div></div></div></div></div>");
  client.println("</body></html>");

  client.println();

  header = "";
  body = "";
  client.stop();
  Serial.println("Client disconnected.");
  Serial.println("");
}

void loop() {
  if (digitalRead(buttonPin) == HIGH) {
    servo();
  }

  if ((millis() - lastFeedingTimeMillis >= feedingInterval) && (time(nullptr) - lastFeedingTimeRTC >= feedingInterval / 1000)) {
    servo();
    lastFeedingTimeMillis = millis();
    lastFeedingTimeRTC = time(nullptr); // Update lastFeedingTimeRTC to current time
  }

  if (analogRead(pinLDR) <= 3000) {
    digitalWrite(pinLED, HIGH);
    delay(200);
    ldrState = "on";
    lcd.setCursor(0, 0);
    lcd.print("Refilling      ");
    lcd.setCursor(0, 1);
    lcd.print("Needed         ");
  } 
  else {
    digitalWrite(pinLED, LOW);
    delay(200);
    ldrState = "off";
    lcd.clear();
  }

  WiFiClient client = server.available();
  if (client) {
    handleClient(client);
  }
}

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

#define SERVO_PIN D4
Servo servo;

const char* ssid = "ServoControl";
const char* password = "12345678";

ESP8266WebServer server(80);

// Enhanced calibration structure
struct ModeCalibration {
  int commandMax;
  int physicalMax;
  int pulseCorrection; // Additional pulse width adjustment
};

ModeCalibration modes[] = {
  {90, 87, 0},    // 90° mode
  {120, 117, 0},  // 120° mode
  {180, 180, 30}  // 180° mode (added pulse correction)
};

int currentMode = 0;
int angle = 0;
int lastAngle = -1;

// Function prototypes
void resetToZero();
void moveServo(int targetAngle);

void moveServo(int targetAngle); // Ensure prototype is visible before lambdas

void setup() {
  Serial.begin(115200);
  
  // Initialize servo with extended pulse range for 180° capability
  servo.attach(SERVO_PIN, 500, 2400 + modes[2].pulseCorrection);
  resetToZero();
  
  WiFi.softAP(ssid, password);
  
    server.on("/", HTTP_GET, [](){
    String html = "<!DOCTYPE html>\n"
      "<html>\n"
      "<head>\n"
      "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
      "  <style>\n"
      "    body { font-family: Arial; text-align: center; margin-top: 20px; }\n"
      "    input { width: 80%; margin: 15px 0; }\n"
      "    button { \n"
      "      padding: 10px 20px; \n"
      "      margin: 5px;\n"
      "      background: #4CAF50; \n"
      "      color: white; \n"
      "      border: none; \n"
      "    }\n"
      "    .active { background: #f44336; }\n"
      "  </style>\n"
      "  <script>\n"
      "    function updateAngle(val) {\n"
      "      document.getElementById(\"angle\").innerHTML = val;\n"
      "      fetch(\"/set?angle=\" + val);\n"
      "    }\n"
      "    function setMaxAngle(mode) {\n"
      "      fetch(\"/setMax?mode=\" + mode).then(() => {\n"
      "        location.reload();\n"
      "      });\n"
      "    }\n"
      "  </script>\n"
      "</head>\n"
      "<body>\n"
      "  <h1>Servo Control</h1>\n"
      "  <input type=\"range\" min=\"0\" max=\"" + String(modes[currentMode].commandMax) + "\" value=\"0\" "
      "oninput=\"updateAngle(this.value)\">\n"
      "  <p>Angle: <span id=\"angle\">0</span>&deg;</p>\n"
      "  <div>\n"
      "    <button class=\"max-btn " + String(currentMode == 0 ? "active" : "") + "\" onclick=\"setMaxAngle(0)\">90&deg; Mode</button>\n"
      "    <button class=\"max-btn " + String(currentMode == 1 ? "active" : "") + "\" onclick=\"setMaxAngle(1)\">120&deg; Mode</button>\n"
      "    <button class=\"max-btn " + String(currentMode == 2 ? "active" : "") + "\" onclick=\"setMaxAngle(2)\">180&deg; Mode</button>\n"
      "  </div>\n"
      "</body>\n"
      "</html>\n";
    server.send(200, "text/html", html);
  });

  server.on("/set", HTTP_GET, [](){
    angle = server.arg("angle").toInt();
    angle = constrain(angle, 0, modes[currentMode].commandMax);
    
    if (angle != lastAngle) {
      moveServo(angle);
      lastAngle = angle;
    }
    server.send(200, "text/plain", String(angle));
  });

  server.on("/setMax", HTTP_GET, [ & ](){
    int newMode = server.arg("mode").toInt();
    if (newMode >= 0 && newMode <= 2) {
      currentMode = newMode;
      // Reattach servo with correct pulse width for the mode
      servo.detach();
      if (currentMode == 2) {
        servo.attach(SERVO_PIN, 500, 2400 + modes[2].pulseCorrection);
      } else {
        servo.attach(SERVO_PIN, 500, 2400);
      }
      resetToZero();
    }
    server.send(200, "text/plain", "Mode set to " + String(modes[currentMode].commandMax) + "°");
  });
  
  server.begin();
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
}

void moveServo(int targetAngle) {
  // Enhanced movement with double-check positioning
  int compensatedAngle = map(targetAngle, 0, modes[currentMode].commandMax, 0, modes[currentMode].physicalMax);
  
  servo.write(compensatedAngle);
  delay(350); // Increased delay for 180° movements
  
  // For 180° mode, add an extra push if needed
  if (currentMode == 2 && targetAngle == 180) {
    servo.write(compensatedAngle + 5); // Small nudge
    delay(100);
    servo.write(compensatedAngle);
  }
}

void resetToZero() {
  servo.write(0);
  delay(500);
  angle = 0;
  lastAngle = -1;
}
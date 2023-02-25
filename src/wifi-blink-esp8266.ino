#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define WIFI_SSID "Leo_Smart_IoT"
#define WIFI_PASSWORD "12345678"

bool ledOn = false;
bool blinkOn = false;
uint32_t blinkInterval;
uint32_t blinkTimer;

ESP8266WebServer server(80);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  server.on("/", onConnect);
  server.on("/on", turnOn);
  server.on("/off", turnOff);
  server.on("/slow", blinkSlow);
  server.on("/fast", blinkFast);
  server.begin();
  delay(100);

  Serial.begin(115200);
  Serial.println("");
  Serial.println("===================");
  Serial.println("HTTP Server started");
  Serial.print("Please visit: http://");
  Serial.print(WiFi.softAPIP());
  Serial.println("/");
}

void onConnect() {
  String content = "<!DOCTYPE HTML>";
  content += "<html>";
  content += "<head>";
  content += "<meta charset=\"utf-8\">";
  content += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  content += "<title>WiFi LED Control</title>";
  content += "<style>";
  content += "body { font-family: sans-serif; text-align: center; padding-top: 50px; }";
  content += "button { font-size: 28px; display: block; margin: 30px auto 0; width: 80%; }";
  content += "</style>";
  content += "</head>";
  content += "<body>";
  content += "<h1>WiFi LED Control</h1>";
  content += "<button id=\"on\">Turn On</button>";
  content += "<button id=\"off\">Turn Off</button>";
  content += "<button id=\"slow\">Blink Slow</button>";
  content += "<button id=\"fast\">Blink Fast</button>";
  content += "<br><br><br><code></code>";
  content += "<script>";
  content += "const sendCmd = e => { const xhr = new XMLHttpRequest; xhr.open('get', `/${e.target.id}`); xhr.send(null); e.preventDefault() }";
  content += ";[...document.querySelectorAll('button')].forEach(el => el.addEventListener('click', sendCmd, false))";
  content += ";document.querySelector('code').textContent = (new Date).toISOString()";
  content += "</script>";
  content += "</body>";
  content += "</html>";

  server.send(200, "text/html", content);
}

void turnOn() {
  ledOn = true;
  blinkOn = false;
}

void turnOff() {
  ledOn = false;
  blinkOn = false;
}

void blinkSlow() {
  blinkLed(1000);
}

void blinkFast() {
  blinkLed(300);
}

void blinkLed(uint32_t interval) {
  blinkOn = true;
  blinkInterval = interval;
  blinkTimer = millis();
}

void loop() {
  server.handleClient();
  if (blinkOn) {
    uint32_t currentTime = millis();
    if (currentTime - blinkTimer > blinkInterval) {
      ledOn = !ledOn;
      blinkTimer = currentTime;
    }
  }
  digitalWrite(LED_BUILTIN, ledOn ? LOW : HIGH);
}

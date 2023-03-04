#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define WIFI_SSID "ESP8266_IOT"
#define WIFI_PASSWORD "12345678"
#define BLINK_INTERVAL 255

bool is_blinking = false;
bool led_on = false;
uint32_t timer;

ESP8266WebServer server(80);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  server.on("/", on_connect);
  server.on("/on", turn_on);
  server.on("/off", turn_off);
  server.on("/blink", blink_led);
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

void on_connect() {
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
  content += "<button id=\"blink\">Blink</button>";
  content += "<br><br><br><code></code>";
  content += "<script>";
  content += "const sendXhr = e => { const xhr = new XMLHttpRequest; xhr.open('get', `/${e.target.id}`); xhr.send(null) }";
  content += ";[...document.querySelectorAll('button')].forEach(btn => btn.addEventListener('click', sendXhr, false))";
  content += ";document.querySelector('code').textContent = (new Date).toISOString()";
  content += "</script>";
  content += "</body>";
  content += "</html>";

  server.send(200, "text/html", content);
}

void turn_on() {
  is_blinking = false;
  led_on = true;
}

void turn_off() {
  is_blinking = false;
  led_on = false;
}

void blink_led() {
  is_blinking = true;
  timer = millis();
}

void loop() {
  server.handleClient();
  if (is_blinking) {
    uint32_t current = millis();
    if (current - timer > BLINK_INTERVAL) {
      led_on = !led_on;
      timer = current;
    }
  }
  digitalWrite(LED_BUILTIN, led_on ? LOW : HIGH);
}

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>

#define AP_SSID "ESP8266_CUBE"
#define AP_PASS "12345678"
#define MAX_LENGTH 21
#define PIXEL_INTEVAL 1000

Adafruit_NeoPixel neon = Adafruit_NeoPixel(36, D2, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel neon2 = Adafruit_NeoPixel(64, D1, NEO_RGB + NEO_KHZ800);
uint32_t black = neon.ColorHSV(0, 0, 0);
uint32_t red = neon.ColorHSV(0, 255, 127);
uint32_t cyan = neon.ColorHSV(35535, 255, 127);

char sta_ssid[MAX_LENGTH];
char sta_pass[MAX_LENGTH];
int addr = 0;

ESP8266WebServer server(80);

bool restoreStaData() {
  for (int i = 0; i < MAX_LENGTH && EEPROM.read(addr + i) != '\0'; i++) {
    sta_ssid[i] = EEPROM.read(addr + i);
  }
  for (int j = 0; j < MAX_LENGTH && EEPROM.read(addr + 24 + j) != '\0'; j++) {
    sta_pass[j] = EEPROM.read(addr + 24 + j);
  }

  if (strlen(sta_ssid) > 4 && strlen(sta_pass) > 4) {
    Serial.println("=========================");
    Serial.print("WiFi: `");
    Serial.print(sta_ssid);
    Serial.print("`. Password: `");
    Serial.print(sta_pass);
    Serial.println("`.");
    Serial.println("=========================");

    return true;
  } else {
    return false;
  }
}

void saveStaData(String param_ssid, String param_pass) {
  const char *ssid = param_ssid.c_str();
  const char *pass = param_pass.c_str();
  int i;
  int j;

  for (i = 0; i < strlen(ssid); i++) {
    EEPROM.write(addr + i, ssid[i]);
    sta_ssid[i] = ssid[i];
  }
  EEPROM.write(addr + i, '\0');
  sta_ssid[i] = '\0';

  for (j = 0; j < strlen(pass); j++) {
    EEPROM.write(addr + 24 + j, pass[j]);
    sta_pass[j] = pass[j];
  }
  EEPROM.write(addr + 24 + j, '\0');
  sta_pass[j] = '\0';

  EEPROM.commit();
}

void handleRoot() {
  String content = "<!DOCTYPE HTML>";
  content += "<h1>ESP8266 AP and Web Server</h1>";
  content += "<p>Enter the credentials of the Wi-Fi network you want to connect to:</p>";
  content += "<form method='POST' action='/connect'>";
  content += "<label for='ssid'>SSID:</label><input type='text' id='ssid' name='ssid' maxlength='20'><br>";
  content += "<label for='pass'>Password:</label><input type='text' id='pass' name='pass' maxlength='20'><br>";
  content += "<input type='submit' value='Connect'>";
  content += "</form>";
  server.send(200, "text/html", content);

  neon.setPixelColor(1, cyan);
  neon.show();
  delay(PIXEL_INTEVAL);
}

void handleConnect() {
  String param_ssid = server.arg("ssid");
  String param_pass = server.arg("pass");
  saveStaData(param_ssid, param_pass);

  server.send(200, "text/html", "<h1>ESP8266 AP and Web Server</h1><p>Connecting to " + param_ssid + "...</p>");

  // Disconnect from the AP mode
  WiFi.softAPdisconnect(true);
  neon.setPixelColor(1, black);
  neon.show();
  delay(PIXEL_INTEVAL);

  // Connect to the STA mode
  startSTA(3);
}

void startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  neon.setPixelColor(1, red);
  neon.show();
  delay(PIXEL_INTEVAL);

  // Print the IP address
  Serial.println("Please visit: http://" + WiFi.softAPIP().toString());
}

void startSTA(int retry) {
  int wait = 5;
  WiFi.begin(sta_ssid, sta_pass);
  neon.setPixelColor(2, red);
  neon.show();
  delay(PIXEL_INTEVAL);

  // Wait for the connection
  while (wait > 0 && WiFi.status() != WL_CONNECTED) {
    wait--;
    delay(1000);
  }

  if (wait > 0) {
    Serial.print("Connected to ");
    Serial.print(sta_ssid);
    Serial.println(". Please visit: http://" + WiFi.localIP().toString());
    neon.setPixelColor(2, cyan);
    neon.show();
    delay(PIXEL_INTEVAL);
  } else {
    Serial.print("Fail to connect WiFi AP ");
    Serial.println(sta_ssid);
    if (retry > 0) {
      startSTA(retry - 1);
    } else {
      WiFi.disconnect(true);
      neon.setPixelColor(2, black);
      neon.show();
      delay(PIXEL_INTEVAL);
      // Restart as an AP
      startAP();
    }
  }
}

void startService() {
  server.on("/", handleRoot);
  server.on("/connect", handleConnect);
  server.begin();
  delay(100);

  neon.setPixelColor(0, black);
  neon.show();
  neon2.setPixelColor(0, black);
  neon2.setPixelColor(1, cyan);
  neon2.setPixelColor(2, red);
  neon2.setPixelColor(3, cyan);
  neon2.show();
  delay(PIXEL_INTEVAL);
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  neon.begin();
  neon2.begin();
  delay(500);

  neon.setPixelColor(0, red);
  neon.show();
  neon2.setPixelColor(0, red);
  neon2.show();
  delay(PIXEL_INTEVAL);

  if (restoreStaData()) {
    startSTA(3); // Set the ESP8266 as a station
  } else {
    startAP(); // Set the ESP8266 as an access point
  }
  startService();
}

void loop() {
  server.handleClient();
}

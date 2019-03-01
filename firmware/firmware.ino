#include <Wire.h>
#include <FastLED.h>
#include <FastLED_NeoMatrix.h>

#define PORT 3000

#ifdef ESP8266
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
ESP8266WebServer server(PORT);
#else
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
WebServer server(PORT);
#endif

#include "secrets.h"

#define PIN_DOWN 27
#define PIN_UP 33
#define PIN_PRESET_2 13
#define PIN_NEOMATRIX 14

#define UP "UP"
#define DOWN "DOWN"
#define PRESET_2 "PRESET_2"

// Neomatrix configuration
#define MATRIX_TILE_WIDTH 8  // width of each individual matrix tile
#define MATRIX_TILE_HEIGHT 4 // height of each individual matrix tile
#define MATRIX_TILE_H 1      // number of matrices arranged horizontally
#define MATRIX_TILE_V 1      // number of matrices arranged vertically
#define mw (MATRIX_TILE_WIDTH * MATRIX_TILE_H)
#define mh (MATRIX_TILE_HEIGHT * MATRIX_TILE_V)
#define NUMMATRIX (mw * mh)

int x = mw;
int pass = 0;
String direction = UP;
String delayMS;
int pin;

uint8_t matrix_brightness = 40;
CRGB matrixleds[NUMMATRIX];
FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(
    matrixleds, MATRIX_TILE_WIDTH,
    MATRIX_TILE_HEIGHT,
    MATRIX_TILE_H,
    MATRIX_TILE_V,
    NEO_MATRIX_BOTTOM + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE + NEO_TILE_TOP + NEO_TILE_LEFT + NEO_TILE_ZIGZAG);

void setup() {
  Serial.begin(9600);

  FastLED.addLeds<NEOPIXEL, PIN_NEOMATRIX>(matrixleds, NUMMATRIX);
  matrix->begin();
  matrix->setBrightness(matrix_brightness);
  matrix->fillScreen(0);
  matrix->show();

  pinMode(PIN_DOWN, OUTPUT);
  pinMode(PIN_UP, OUTPUT);
  pinMode(PIN_PRESET_2, OUTPUT);

  digitalWrite(PIN_DOWN, LOW);
  digitalWrite(PIN_UP, LOW);
  digitalWrite(PIN_PRESET_2, LOW);

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

#ifdef ESP8266
  if (MDNS.begin("esp8266")) {
#else
  if (MDNS.begin("esp32")) {
#endif
    Serial.println("MDNS responder started");
  }

  server.on("/move", handleMove);
  server.onNotFound(handleNotFound);

  const char *headerkeys[] = { "direction", "ms" };
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
  server.collectHeaders(headerkeys, headerkeyssize);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // TODO
  // Serial.parseInt();
  logDeviceData();

  server.handleClient();

  delay(125);
}

void handleNotFound() {
  String message = "Route Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

void logDeviceData() {
  Serial.print("Device IP: ");
  Serial.println(WiFi.localIP());

  Serial.print("Device MAC: ");
  Serial.println(WiFi.macAddress());

  Serial.print("Memory heap: ");
  Serial.println(ESP.getFreeHeap());
}

// TODO
void drawUp() {
  matrixleds[1].setRGB(30, 30, 0);
  matrixleds[2].setRGB(30, 30, 0);
  matrixleds[3].setRGB(30, 30, 0);
  matrixleds[9].setRGB(30, 30, 0);
  matrixleds[10].setRGB(30, 30, 0);
  matrixleds[11].setRGB(30, 30, 0);
  matrix->show();
}

void drawDown() {
  matrixleds[1].setRGB(0, 30, 0);
  matrixleds[2].setRGB(0, 30, 0);
  matrixleds[3].setRGB(0, 30, 0);
  matrixleds[9].setRGB(0, 30, 0);
  matrixleds[10].setRGB(0, 30, 0);
  matrixleds[11].setRGB(0, 30, 0);
  matrix->show();
}

void drawPreset2() {
  matrixleds[1].setRGB(0, 30, 0);
  matrixleds[2].setRGB(30, 0, 0);
  matrixleds[3].setRGB(0, 0, 30);
  matrixleds[9].setRGB(30, 30, 0);
  matrixleds[10].setRGB(30, 0, 30);
  matrixleds[11].setRGB(0, 30, 30);
  matrix->show();
}

void writeInt(unsigned int value){
    Wire.write(lowByte(value));
    Wire.write(highByte(value));
}

void handleMove() {
  Serial.println("Move request received");
  String header;
  String content = "Successfully connected to device. ";

  if (server.hasHeader("direction")) {
    direction = server.header("direction");
    delayMS = server.header("ms");
    Serial.print("DELAY: ");
    Serial.println(delayMS);

    // HTTP response
    content += "Target: " + direction + " for " + delayMS + "ms";

    // Serial logging
    Serial.print("Requested to move to: "); Serial.println(direction); Serial.println(" for "); Serial.println(delayMS); Serial.println("ms");

    if (direction == UP) {
      drawUp();
      pin = PIN_UP;
    }
    if (direction == DOWN) {
      drawDown();
      pin = PIN_DOWN;
    }
    if (direction == PRESET_2) {
      drawPreset2();
      pin = PIN_PRESET_2;
    }

    server.send(200, "text/html", content);

    digitalWrite(pin, HIGH);
    delay(delayMS.toInt());
    digitalWrite(pin, LOW);

    matrix->fillScreen(0);
    matrix->show();
  } else {
    content += "No \"direction\" header found.";
    server.send(500, "text/html", content);
  }
}

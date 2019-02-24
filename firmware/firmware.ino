// Partially adapted from https://github.com/bbx10/WebServer_tng/blob/master/examples/AdvancedWebServer/AdvancedWebServer.ino

#include <FastLED.h>
#include <FastLED_NeoMatrix.h>

#ifdef ESP8266
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
ESP8266WebServer server(80);
#else
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
WebServer server(80);
#endif

#include "secrets.h"

#define PIN 2

#define UP "UP"
#define DOWN "DOWN"

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
char direction[64] = UP;

uint8_t matrix_brightness = 40;
CRGB matrixleds[NUMMATRIX];
FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(
    matrixleds, MATRIX_TILE_WIDTH,
    MATRIX_TILE_HEIGHT,
    MATRIX_TILE_H,
    MATRIX_TILE_V,
    NEO_MATRIX_BOTTOM + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE + NEO_TILE_TOP + NEO_TILE_LEFT + NEO_TILE_ZIGZAG);

const uint16_t colors[] = {
  matrix->Color(255, 0, 0),
  matrix->Color(0, 255, 0),
  matrix->Color(0, 0, 255)
};

void setup() {
  Serial.begin(115200);

  FastLED.addLeds<NEOPIXEL, PIN>(matrixleds, NUMMATRIX);
  matrix->begin();
  matrix->setBrightness(matrix_brightness);

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

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  const char *headerkeys[] = { "direction" };
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
  server.collectHeaders(headerkeys, headerkeyssize);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  logDeviceData();

// TODO uncomment and move drawArrow into server handler
//  server.handleClient();

  drawArrow();

  delay(125);
}

void handleRoot() {
  Serial.println("Enter handleRoot");
  String header;
  String content = "Successfully connected to device. ";

  if (server.hasHeader("direction")) {
    content += "Direction: " + server.header("direction");

    server.header("direction").toCharArray(direction, 64);
    server.send(200, "text/html", content);
  } else {
    content += "No \"direction\" header found.";
    server.send(500, "text/html", content);
  }
}

void logDeviceData() {
  Serial.print("Device IP: ");
  Serial.println(WiFi.localIP());

  Serial.print("Device MAC: ");
  Serial.println(WiFi.macAddress());

  Serial.print("Memory heap: ");
  Serial.println(ESP.getFreeHeap());
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

// TODO make into an arrow
void drawArrow() {
  matrixleds[1].setRGB(0, 100, 0);
  matrixleds[2].setRGB(100, 0, 0);
  matrixleds[3].setRGB(0, 0, 100);
  matrix->show();
}


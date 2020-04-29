

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "gamma.h"
#include "preset.h"
#define LED_COUNT ROW_LENGTH * COLUMN_LENGTH

ESP8266WebServer server(80);

uint16_t *setting = preset;

Adafruit_NeoPixel strip(LED_COUNT, LED_PANEL_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoMatrix matrix(ROW_LENGTH, ROW_LENGTH, LED_PANEL_PIN, NEO_MATRIX_BOTTOM  + NEO_MATRIX_RIGHT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG);
  
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(LED_PANEL_PIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(2, HIGH);
  matrix.begin();
  matrix.setBrightness(50);
  matrix.setTextColor(cream);
  matrix.fillScreen(teal);
  matrix.setTextWrap(false);
  matrix.show();
  Serial.begin(9600);
  Serial.println("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  
  Serial.println(WiFi.localIP());
  server.on("/", HTTP_OPTIONS, []() {
    server.sendHeader("Access-Control-Max-Age", "10000");
    server.sendHeader("Access-Control-Allow-Methods", "POST,GET,OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "*");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "" );
  });
  server.on("/", HTTP_GET, handle_connect);
  server.on("/", HTTP_POST, handle_post);
  server.onNotFound(handle_NotFound);
  server.begin();
  String text =  WiFi.localIP().toString();
  display_text("IP:" + text, 85, 2, 0xffff, 0x0208);
  Serial.println("Server started.");
  matrix.show();
  matrix.setBrightness(255);
  fill_design(setting);
}

void loop() {
  server.handleClient();
}

void handle_connect(){
  Serial.println("request recieved");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "get response"); 
}

void handle_post(){
  const int capacity = JSON_ARRAY_SIZE(LED_COUNT) + LED_COUNT * 8;
  DynamicJsonDocument doc(capacity);
  DeserializationError err = deserializeJson(doc, server.arg("plain"));
  if(err) {
    Serial.print(F("deserializeJson() failed with code "));
    Serial.println(err.c_str());
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(400, "text/plain", "error, unable to set colors");
  } else {
    for(int i = 0; i < LED_COUNT; i++){
      const char* color = doc[i];
      setting[i] = convert_color((uint32_t)strtol(color + 1, NULL, 16));
    }
    fill_design(setting);
    strip.show();
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "success!!");
  }
}

void handle_NotFound(){
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(404, "text/plain", "Not found");
}

void display_text(String text, int scroll_interval, int repeats, uint32_t text_color, uint32_t background_color){
  int text_length = text.length() * 7;
  matrix.setCursor(ROW_LENGTH, 3);
  matrix.setTextColor(text_color); 
  matrix.print(text);
  matrix.show();
  int i = ROW_LENGTH;
  while(i > ROW_LENGTH - text_length){
    matrix.fillScreen(0x0208);
    matrix.setCursor(--i, 3);
    matrix.print(text);
    matrix.show();
    delay(scroll_interval);
  }
}

void fill_design(uint16_t setting[LED_COUNT]){
  for(int y = 0; y < COLUMN_LENGTH; y++){
    for(int x = 0; x < ROW_LENGTH; x++){
      matrix.drawPixel(x, y, setting[y * ROW_LENGTH + x]);      
    }
  }
  matrix.show();
}

uint16_t convert_color(uint32_t color){
  int r = color>>16;
  int g = color - (r<<16)>>8;
  int b = color - (r<<16) - (g<<8);
  r = pgm_read_byte(&gamma8[r]);
  g = pgm_read_byte(&gamma8[g]);
  b = pgm_read_byte(&gamma8[b]);
  r = r>>3;
  g = g>>2;
  b = b>>3;
  uint16_t color_out = (r<<11) + (g<<5) + b;
  return color_out;
}

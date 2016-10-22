#include <ESP8266WiFi.h>
#include "constants.h"

WiFiServer* server;

bool pressed = false;

String bridge;

String get_ip(String l) {
  String out;
  const char* key = "\"internalipaddress\"";
  char* keystart = strstr(l.c_str(), key);
  if(keystart == NULL) {
    return out;
  }
  char* keyend = keystart + strlen(key);
  char* ipstart = strchr(keyend, '"');
  if(ipstart == NULL) {
    return out;
  }
  ipstart += 1;
  char* ipend = strchr(ipstart, '"');
  if(ipend == NULL) {
    return out;
  }
  out += ipstart;
  return out.substring(0, ipend-ipstart);
}

String get_bridge() {
  const char* hue = "www.meethue.com";
  Serial.println("Retrieving local Hue Bridge ip");
  WiFiClientSecure client;
  String out;
  while(true) {
    if(client.connect(hue, 443)) {
      Serial.println("Connection Established");
      client.print(String("GET /api/nupnp HTTP/1.1\r\n") +
        "Host: " + hue + "\r\n" +
        "Connection: close\r\n\r\n");
      while(client.connected()) {
        if(client.available()) {
          String l = client.readStringUntil('\n');
          String ip = get_ip(l);
          if(ip.length() != 0) {
            Serial.println("FOUND IP");
            out = ip;
          }
        }
      }
      Serial.println("Connection Finished");
    }else {
      Serial.println("Connection Failed");
    }
    client.stop();
    if(out.length() != 0) {
      return out;
    }
    delay(500);
  }
}

void setup() {
  pinMode(D8, INPUT);
  Serial.begin(115200);
  Serial.println("STARTING");
  WiFi.mode(WIFI_OFF);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println(WiFi.status());
    delay(2000);
  }
  Serial.println();
  Serial.print("Connected: ");
  Serial.println(WiFi.localIP());
  // put your setup code here, to run once:
  bridge = get_bridge();
  Serial.println(String("Hue Bridge: ") + bridge);
  server = new WiFiServer(80);
  server->begin();
}

void test_light(){
  WiFiClient cl;
  if(cl.connect(bridge.c_str(), 80)) {
    String body = String("{\"scene\":\"")+SCENE_ID+"\"}\r\n";
    cl.print(String("PUT /api/")+ USERNAME + "/groups/1/action HTTP/1.1\r\n" +
      "Connection: close\r\n" +
      "Content-Length: "+body.length()+"\r\n"+
      "Content-type: application/json\r\n" +
      "Host: "+bridge+"\r\n"+
      "\r\n" + body
      );
    while(cl.connected()){
      if(cl.available()){
        Serial.println(cl.readStringUntil('\n'));
      }
    }
  }else {
    Serial.println(String("Could not connect to bridge at ") + bridge);
  }
}

String report() {
  return String("HTTP/1.1 OK\r\n") +
    "Content-Type: text/html\r\n" +
    "Connection: close\r\n" +
    "\r\n" +
    "HELLO WORLD: " + digitalRead(D8) + " " + D8 + "\r\n";
}

void loop() {
  //Serial.print(".");
  WiFiClient client = server->available();
  if(client) {
    Serial.println("!");
    if(client.connected()) {
      client.println(report());
    }
    while(client.connected()) {
      if(client.available()) {
        String line = client.readStringUntil('\n');
        Serial.println(line);
        if(line.length()==1 && line[0] == '\r') {
          break;
        }
      } else {
        Serial.println("WHAT");
        break;
      }
    }
    while(client.available()){
      client.readStringUntil('\r');
    }
    delay(1);
    client.stop();
    Serial.println("[Disconnected]");
  }
  bool digit = digitalRead(D8);
  if(digit) {
    test_light();
  }
  pressed = digit;
  delay(10);
  // put your main code here, to run repeatedly:

}

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WebSocketsClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "arduino_base64.hpp"



// Налаштування Wi-Fi
const char* ssid = "GrafWiFi";
const char* password = "G03G07m06";


// Налаштування WebSocket
const char* websocket_server ="proxy.wynd.network";
const uint16_t websocket_port = 4650;
const char* websocket_path = "/";
WebSocketsClient webSocket;


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

String user_id = "";
String device_id ="";

bool isConnected = false;

long int ping_int = 60000;
long int last_ping;




String uuidv4(int seed) {
  randomSeed(seed);
  String lettars = "0123456789abcdefghijklmnopqrstuvwxyz";
  String out = "";
  for (int i = 0; i < 30; i++) {
      if (i == 8 || i == 15 || i == 18) out+="-";
      if (i == 12) out += "-4";
      if (i == 15) out += lettars[random(8, 12)];
      out += lettars[random(36)];
  }
  return out;
}

unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}


int send_request(String url, JsonObject headers, String method, String user_agent, String *body64, String *date, String *etag) {
  WiFiClient client;
  HTTPClient http;
  http.begin(client, url);

  for (JsonPair kv : headers) {
    http.addHeader(kv.key().c_str(), kv.value().as<String>());  
  }

  const char * headerKeys[] = {"date", "etag"};
  http.collectHeaders(headerKeys, 2);
  http.setUserAgent(user_agent);
  
  

  int httpResponseCode;
  if (method == "GET") {
    httpResponseCode = http.GET();
  } else {
    httpResponseCode = http.POST("");
  }

  if (httpResponseCode>0) {
    String payload = http.getString();

    yield();

    char output[base64::encodeLength(payload.length())];
    base64::encode((const uint8_t*)payload.c_str(), payload.length(), output);
    *body64 = output;

    yield();
    
    *date = http.header("date");

    *etag = http.header("etag");
    
    yield();

    http.end();
    return 0;

  } else {
    http.end();
    return -1;
  }
}

void HTTP_req(JsonDocument doc){
  String outpayload, date, etag;
  JsonDocument req_data = doc["data"];
  String url = req_data["url"];
  JsonObject headerss = req_data["headers"];
  String method = req_data["method"];
  String user = headerss["User-Agent"];
  int status_code = send_request(url, headerss, method, user,  &outpayload, &date, &etag);
  JsonDocument send_doc;
  send_doc["id"] = doc["id"];
  send_doc["origin_action"] = "HTTP_REQUEST";
  JsonObject result = send_doc.createNestedObject("result");
  result["url"] = url;
  result["status"] = 200;
  result["status_text"] = "OK";
  JsonObject headers = result.createNestedObject("headers");
  headers["x-powered-by"] = "Express";
  headers["content-type"] = "application/json; charset=utf-8";
  headers["content-length"] = "29";
  headers["etag"] = etag;
  headers["date"] = date;
  headers["connection"] = "keep-alive";
  headers["keep-alive"] = "timeout=5";
  result["body"] = outpayload;

  String jsonStr;
  serializeJson(send_doc, jsonStr);
  webSocket.sendTXT(jsonStr);
  Serial.println("Sent HTTP_Request: " + jsonStr);
}

void ConectedWebSoCket() {
  yield();
  Serial.println("Conecting to websoket");
  webSocket.beginSSL(websocket_server, websocket_port, websocket_path, NULL, "");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
  webSocket.enableHeartbeat(60000, 60000, 3);
  yield();
  blink(200, 750);
  blink(200, 750);
}

// Обробка подій WebSocket
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    blink(200, 200);
    blink(200, 200);
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.println("Disconnected from WebSocket!");
            isConnected = false;
            break;
            
        case WStype_CONNECTED:
            Serial.println("Connected to WebSocket!");
            isConnected =  true;
            break;

        case WStype_TEXT: {
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, payload);
            serializeJson(doc, Serial);
            String action = doc["action"].as<String>();
            Serial.println("");
            if (action == "AUTH") {
                sendAuth(doc["id"].as<String>());
            } else if (action == "PONG") {
                sendPong(doc["id"].as<String>());
            } else if (action == "HTTP_REQUEST")
                HTTP_req(doc);
            break;
        }
        case WStype_ERROR:
            Serial.println("WebSocket Error!");
            break;

        default:
            Serial.println(type);
            break;
    }
    blink(200, 200);
    blink(200, 200);
}

void sendPing() {
    String jsonStr = "{\"id\":\""+uuidv4(analogRead(0))+"\",\"version\":\"1.0.0\",\"action\":\"PING\",\"data\":{}}";
    webSocket.sendTXT(jsonStr);
    Serial.println("Sent PING: " + jsonStr);
}

void sendAuth(String id) {
    DynamicJsonDocument doc(512);
    doc["id"] = id;
    doc["origin_action"] = "AUTH";
    JsonObject result = doc.createNestedObject("result");
    result["browser_id"] = device_id;
    result["user_id"] = user_id;
    result["user_agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36";
    result["timestamp"] = getTime();
    result["device_type"] = "desktop";
    result["version"] = "4.30.0";

    String jsonStr;
    serializeJson(doc, jsonStr);
    webSocket.sendTXT(jsonStr);
    Serial.println("Sent AUTH: " + jsonStr);
}


void sendPong(String id) {
    DynamicJsonDocument doc(256);
    doc["id"] = id;
    doc["origin_action"] = "PONG";
    String jsonStr;
    serializeJson(doc, jsonStr);
    webSocket.sendTXT(jsonStr);
    Serial.println("Sent PONG: " + jsonStr);
}

void blink(int bt, int at) {
  yield();
  delay(bt);
  digitalWrite(LED_BUILTIN, LOW);
  delay(at);
  digitalWrite(LED_BUILTIN, HIGH);
  yield();
}


void setup() {
  device_id = uuidv4(ESP.getChipId());
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_BUILTIN, OUTPUT);
  ConectedWebSoCket();
  digitalWrite(LED_BUILTIN, LOW);
  delay(3000);
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
  yield();
  if (WiFi.status() != WL_CONNECTED) {
    yield();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");
    Serial.print("Conecting");
    yield();
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      blink(0,200);
    }
    Serial.println("Conected");
    Serial.print("IP Address of network: ");
    Serial.println(WiFi.localIP());
    Serial.println("");
    blink(200, 1000);
    blink(200, 1000);
  } else {
    yield();
    webSocket.loop();
    // if (millis() - last_ping > ping_int && isConnected) {
    //   blink(200, 200);
    //   sendPing();
    //   last_ping = millis();
    //   yield();
    //   blink(200, 200);
    // }
    }
}

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <time.h>


// Налаштування Wi-Fi
const char* ssid = "GrafWiFi";
const char* password = "G03G07m06";


// Налаштування WebSocket
const char* websocket_server ="proxy.wynd.network";
const uint16_t websocket_port = 4650;
const char* websocket_path = "/"; //ws://192.168.1.105:25545
WebSocketsClient webSocket;


// Унікальні ID користувача та пристрою
String user_id = "2ocP7c2meIDATQ28vFgk3IaXZjW";
String device_id = String(ESP.getChipId(), HEX);

bool isConnected = false;

long int ping_int = 20000;
long int last_ping;
long int recon_int = 20000;
long int last_recon;

void ConectedWebSoCket() {
  yield();
  Serial.println("Conecting to websoket");
  webSocket.beginSSL(websocket_server, websocket_port, websocket_path, NULL, "");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
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
            // Обробка вхідних повідомлень
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, payload);
            String action = doc["action"].as<String>();
            
            if (action == "AUTH") {
                sendAuth(doc["id"].as<String>());
            } else if (action == "PONG") {
                sendPong(doc["id"].as<String>());
            }

            break;
        }

        case WStype_ERROR:
            Serial.println("WebSocket Error!");
            break;

        default:
            break;
    }
    blink(200, 200);
    blink(200, 200);
}

// Відправка PING повідомлення
void sendPing() {
    DynamicJsonDocument doc(256);
    doc["id"] = String(random(0, 100000)); // Генеруємо унікальний ID
    doc["version"] = "1.0.0";
    doc["action"] = "PING";
    doc["data"] = JsonObject();

    String jsonStr;
    serializeJson(doc, jsonStr);
    webSocket.sendTXT(jsonStr);
    Serial.println("Sent PING: " + jsonStr);
}

// Відправка AUTH відповіді
void sendAuth(String id) {
    DynamicJsonDocument doc(512);
    doc["id"] = id;
    doc["origin_action"] = "AUTH";

    JsonObject result = doc.createNestedObject("result");
    result["browser_id"] = device_id;
    result["user_id"] = user_id;
    result["user_agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36";
    result["timestamp"] = time(nullptr);
    result["device_type"] = "extension";
    result["version"] = "2.5.0";

    String jsonStr;
    serializeJson(doc, jsonStr);
    webSocket.sendTXT(jsonStr);
    Serial.println("Sent AUTH: " + jsonStr);
}

// Відправка PONG відповіді
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
    if (millis() - last_ping > ping_int && isConnected) {
      blink(200, 200);
      sendPing();
      last_ping = millis();
      yield();
      blink(200, 200);
    }
    }
}



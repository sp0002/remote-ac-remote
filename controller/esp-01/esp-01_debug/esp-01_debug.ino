#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

#include <SocketIOclient.h>

#define RR_TOKEN "token=abcdefghijklmnopqrstuvwxyz"
#define WIFI_SSID ""
#define WIFI_PASS ""
#define SOCKET_HOST "192.168.1.0"
#define SOCKET_PORT 5000
#define SOCKET_PORT_STR "5000"

ESP8266WiFiMulti WiFiMulti;
SocketIOclient ioClient;

String cookie = "";
bool reconnect = false;

void ioClientEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case sIOtype_DISCONNECT:
      Serial.println("[WSc] Disconnected!");
      reconnect = true;
      break;
    case sIOtype_CONNECT:
      Serial.printf("[WSc] Connected to url: %s\n",  payload);
      ioClient.send(sIOtype_CONNECT, "/");
      break;
    case sIOtype_EVENT:
      Serial.printf("[WSc] get text: %s\n", payload);
      break;
    }
}

void connect_token(){
  WiFiClient client;
  HTTPClient http;
  cookie = "Cookie: ";

  http.begin(client, "http://" SOCKET_HOST ":" SOCKET_PORT_STR "/token_enter");  // HTTP
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  const char* keys[] = {"Set-Cookie"};
  http.collectHeaders(keys, 1);  // Tells HTTPClient to collect Set-Cookie header to get our session cookie.

  // start connection and send HTTP header and body
  int httpCode = http.POST(RR_TOKEN);

  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] POST code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      // const String& payload = http.getString();  // Response content
      // Get session cookie.
      bool exit = false;
      int index = 0;
      int prev_index = 0;
      while (!exit) {
        index = http.header("Set-Cookie").indexOf(";");
        if (index == -1){
          if (http.header("Set-Cookie").substring(prev_index, index).startsWith("session=")){
            cookie += http.header("Set-Cookie").substring(prev_index, index);
            ioClient.setExtraHeaders(cookie.c_str());
            Serial.print("New Cookie: ");
            Serial.println(cookie);
            reconnect = false;
          } else {
            Serial.println("End of line, session cookie not found.");
          }
          exit = true;
        } else {
          if (http.header("Set-Cookie").substring(prev_index, index).startsWith("session=")){
            cookie += http.header("Set-Cookie").substring(prev_index, index);
            ioClient.setExtraHeaders(cookie.c_str());
            Serial.print("New Cookie: ");
            Serial.println(cookie);
            reconnect = false;
            exit = true;
          } else {
            prev_index = index + 1;
          }
        }
      }
    }
  } else {
    Serial.printf("[HTTP] POST failed, error: %s\n", http.errorToString(httpCode).c_str());
    delay(2000);
  }
  http.end();
  Serial.print("Cookie: ");
  Serial.println(cookie);
}

void setup() {
  Serial.begin(9600);
  delay(6000);  // Allows time for serial monitor to connect to the ESP8266

  Serial.setDebugOutput(true);

  Serial.println("Starting up...");
  Serial.println();

  WiFiMulti.addAP(WIFI_SSID, WIFI_PASS);

  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  connect_token();
  
  ioClient.begin(SOCKET_HOST, SOCKET_PORT, "/socket.io/?EIO=4");
  ioClient.setReconnectInterval(1600);  // In ms
  //ioClient.setAuthorization("user", "Password");  // HTTP Basic Authorization
  ioClient.onEvent(ioClientEvent);
}


void loop() {
  if (!reconnect){
    ioClient.loop();
  } else {
    connect_token();
  }
}

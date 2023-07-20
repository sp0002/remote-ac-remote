#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

#include <SocketIOclient.h>

#define RR_TOKEN "token=abcdefghijklmnopqrstuvwxyz"
#define WIFI_SSID ""
#define WIFI_PASS ""
#define SOCKET_HOST "example.com"
#define SOCKET_PORT 443
#define SOCKET_PORT_STR "443"

#define NTP_SERVER "pool.ntp.org"
#define timezone "<+08>-8"

// This is the root CA PEM (cert) of my HTTPS server which cert is issued by Let's Encrypt.
const char root_CA_cert [] PROGMEM = R"CERT(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)CERT";

X509List cert(root_CA_cert);

ESP8266WiFiMulti WiFiMulti;
SocketIOclient ioClient;

String cookie = "";
bool reconnect = false;

void ioClientEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case sIOtype_DISCONNECT:
      Serial.println("Disconnected!");
      reconnect = true;
      break;
    case sIOtype_CONNECT:
      ioClient.send(sIOtype_CONNECT, "/");
      break;
    case sIOtype_EVENT:
      String data = (char *)payload;
      if (data.startsWith("[\"remote\",\"")){
        data = data.substring(11);
        data = data.substring(0, data.length()-2);
        Serial.println(data);
      }
      break;
    }
}

void connect_token(){
  WiFiClientSecure client;
  HTTPClient http;
  cookie = "Cookie: ";
  client.setTrustAnchors(&cert);

  http.begin(client, "https://" SOCKET_HOST ":" SOCKET_PORT_STR "/token_enter");  // HTTP
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  const char* keys[] = {"Set-Cookie"};
  http.collectHeaders(keys, 1);  // Tells HTTPClient to collect Set-Cookie header to get our session cookie.

  // start connection and send HTTP header and body
  int httpCode = http.POST(RR_TOKEN);

  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled

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
            reconnect = false;
          }
          exit = true;
        } else {
          if (http.header("Set-Cookie").substring(prev_index, index).startsWith("session=")){
            cookie += http.header("Set-Cookie").substring(prev_index, index);
            ioClient.setExtraHeaders(cookie.c_str());
            reconnect = false;
            exit = true;
          } else {
            prev_index = index + 1;
          }
        }
      }
    }
  } else {
    Serial.printf("POST err: %s\n", http.errorToString(httpCode).c_str());
    delay(2000);
  }
  http.end();
}

void setup() {
  Serial.begin(9600);
  delay(6000);  // Allows some time for ESP8266 and Arduino to connect.

  Serial.setDebugOutput(false);

  WiFiMulti.addAP(WIFI_SSID, WIFI_PASS);

  //WiFi.disconnect();
  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(1000);
  }
  configTime(timezone, NTP_SERVER);

  connect_token();
  
  ioClient.beginSSL(SOCKET_HOST, SOCKET_PORT, "/socket.io/?EIO=4");
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

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <PubSubClient.h>

const int RELAYPIN = D1;

String devID = "esp8266-relay";
const char* doorActionTopic = "/garage/door/action";
const char* logInfoTopic = "/log/info";
char message_buff[100];

WiFiClient wifiClient;
PubSubClient client("192.168.1.239", 1883, wifiClient);

void findKnownWiFiNetworks() {
  ESP8266WiFiMulti wifiMulti;
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP("BYU-WiFi", "");
  wifiMulti.addAP("Hancock2.4G", "Arohanui");
  Serial.println("");
  Serial.print("Connecting to Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    wifiMulti.run();
    delay(1000);
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID().c_str());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void pubDebug(String message) {
  client.publish("/log/debug", message.c_str());
}

void sendHassDeviceConfig() {
  client.publish("homeassistant/sensor/garage/door_relay/config", "{\"name\": \"Garage Door Relay\", \"state_topic\": \"/garage/door/action\"}");
}

void setRelay(String message) {
  Serial.println(message);
  if (message == "off") {
    Serial.println("turning off relay");
    digitalWrite(RELAYPIN, LOW);
  } else if (message == "on") { // require explicit closed
    Serial.println("turning on relay");
    digitalWrite(RELAYPIN, HIGH);
  } else {
    pubDebug(String("Wrong relay message received: " + message));  
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  int i = 0;
  for(i = 0; i < length; i++) {
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';
  String message = String(message_buff);
  Serial.println(message);
  setRelay(message);
}

void reconnectMQTT() {
  while(!client.connected()) {
    if (client.connect((char*) devID.c_str(), "mqtt", "mymqttpassword")) {
      Serial.println("Connected to MQTT server");
      String message = devID;
      message += ": ";
      message += WiFi.localIP().toString();
      if (client.publish(logInfoTopic, message.c_str())) {
        Serial.println("published successfully");
      } else {
        Serial.println("failed to publish");
      }
      client.subscribe(doorActionTopic);
      sendHassDeviceConfig();
    } else {
      Serial.println("MQTT connection failed");
      delay(5000);
    }
  }
}

void setup(void) {
  Serial.begin(115200);
  findKnownWiFiNetworks();
  pinMode(RELAYPIN, OUTPUT);
  client.setCallback(callback);
  setRelay("off");
}

void loop(void) {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
}

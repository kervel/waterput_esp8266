#define SERIAL_NUMBER "wl_000001"
#include "wifi_mgmt.h"
#include "ultrasensor.h"


// fwd declarations
void setup_wifi();


unsigned int maxRetries = 20;

WiFiClient espClient;
std::shared_ptr<PubSubClient> client;


void callback(char *topic, byte *payload, unsigned int length) {
  /*Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();*/
}


int state, oldState;
long int loopCount;

void setup() {
  oldState = -1;
  state = -1;
  loopCount = 0;
  setup_sermode();
  setup_wifi();
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  client = setup_wifi_and_mqtt(callback);
  // pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  if (!WiFi.isConnected()) {
    Serial.println("wifi connection lost, rebooting");
    ESP.restart();
  }
  if (!client->connected()) {
    Serial.print("MQTT connection lost, rebooting...");
    ESP.restart();
  }
}




void loop() {
  char charBuf[50];
  reconnect();

  if (maxRetries == 0) {
    Serial.println("too many failures sending, restart");
    ESP.restart();
  }

  loopCount += 1;

  int newState = getdist_sermode();
  // take the maximum distance found. other distances are probably noise
  state = max(state, newState);
  if (loopCount % 60 == 0) {
    Serial.print("got new state ");
    Serial.print(state);
    Serial.println("..");

    if ((state != oldState) && (state > 0)) {
      itoa(state, charBuf, 10);
      if (!client->publish(unique_topic("state").c_str(), charBuf)) {
        maxRetries -= 1;
      }
      oldState = state;
    }
    state = -1;
  }
  if (loopCount % 600 == 0) {
    ltoa(loopCount, charBuf, 10);
    if (!client->publish(online_topic().c_str(), charBuf, true)) {
      maxRetries -= 1;
    }
  }
  client->loop();
  
}

#define SERIAL_NUMBER "wl_000001"
#include "ultrasensor.h"
#include "wifi_mgmt.h"

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

double state, oldState;
long int loopCount, localLoop;

#define MEAS_INT 30
double measurements[MEAS_INT];

void setup() {
  oldState = -1;
  state = -1;
  loopCount = 0;
  localLoop = 0;
  Serial.begin(9600);
  setup_sermode();
  setup_wifi();
  for (int i = 0; i < MEAS_INT; i++) {
    measurements[i] = 0;
  }
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

int cmpfunc(const void *a, const void *b) {
  return (*(double *)a - *(double *)b);
}

void loop() {
  char charBuf[50];
  reconnect();

  if (maxRetries == 0) {
    Serial.println("too many failures sending, restart");
    ESP.restart();
  }

  loopCount += 1;

  // take the maximum distance found. other distances are probably noise
  if (localLoop >= MEAS_INT) {
    localLoop = 0;
    qsort(measurements, MEAS_INT, sizeof(double), cmpfunc);
    state = measurements[MEAS_INT / 2];
    Serial.print("got new state ");
    Serial.print(state);
    Serial.println("..");

    if ((state != oldState) && (state > 0)) {
      if (!client->publish(unique_topic("state").c_str(),
                           String(state).c_str())) {
        maxRetries -= 1;
      }
      oldState = state;
    }
    state = -1;
    for (int i = 0; i < MEAS_INT; i++) {
      measurements[i] = 0;
    }
  } else {
    double newState = getdist_triggermode();
    if (newState > 0) {
      measurements[localLoop] = newState;
      localLoop += 1;
      Serial.print("[");
      Serial.print(localLoop);
      Serial.print("]=");
      Serial.println(newState);
    }
  }
  if (loopCount % 600 == 0) {
    ltoa(loopCount, charBuf, 10);
    if (!client->publish(online_topic().c_str(), charBuf, true)) {
      maxRetries -= 1;
    }
  }
  client->loop();
}

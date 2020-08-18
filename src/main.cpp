#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>

#include "credentials.h"

#define STATE_TOPIC "/regenwater/state"
#define ONLINE_TOPIC "/regenwater/online"

// fwd declarations
bool setup_wifi();

const int pingPin = D6; // Trigger Pin of Ultrasonic Sensor GPIO12
const int echoPin = D7; // Echo Pin of Ultrasonic Sensor GPIO13

WiFiClient espClient;
PubSubClient client(espClient);

SoftwareSerial sensor; // RX = echoPin
const int MAX_TRY_SERIAL = 50;

void callback(char *topic, byte *payload, unsigned int length) {
  /*Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();*/
}

void setup_sermode() {
  Serial.begin(9600);
  sensor.begin(9600, echoPin, pingPin, SWSERIAL_8N1, false, 16536, 4096);
}

void setup_triggermode() {
  pinMode(pingPin, OUTPUT);
  pinMode(echoPin, INPUT);
  Serial.begin(9600);
}

int state, oldState;
unsigned int maxRetries;
long int loopCount;

void setup() {
  oldState = -1;
  state = -1;
  maxRetries = 20;
  loopCount = 0;
  setup_sermode();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

bool setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network

  // pinMode(LED_BUILTIN, OUTPUT);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  int internal_retries = 20;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    internal_retries -= 1;
    if (internal_retries < 1) {
      Serial.println("wifi fail, rebooting");
      ESP.restart();
    }
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  int wifiret = 2;
  while (!WiFi.isConnected()) {
    wifiret -= 1;
    if (wifiret < 1) {
      Serial.print("rebooting (reconnect no wifi)");
      ESP.restart();
    }
  }
  int internal_retries = 10;
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    if (client.connect("regenwater", mqtt_username, mqtt_password, ONLINE_TOPIC,
                       0, 1, "0")) {
      Serial.println("connected");
    } else {
      internal_retries -= 1;
      if (internal_retries < 1) {
        Serial.println("ESP fail, rebooting");
        ESP.restart();
      }
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

long microsecondsToCentimeters(long microseconds) {
  return microseconds / 29 / 2;
}

int getdist_triggermode() {
  long duration, cm;
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(pingPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  cm = microsecondsToCentimeters(duration);
  Serial.print(cm);
  Serial.print("cm");
  Serial.println();
  delay(100);
  return cm;
}

// source:
// http://www.setfirelabs.com/building-automation/dyp-me007y-tx-serial-output-ultrasonic-sensor-interfacing-with-arduino-nodemcu
int getDistance() {
  byte msb, lsb, checksum, checkcalc, tries = 0;
  int distance;

  // we want a 255 which is the start of the reading (not msb but saving a byte
  // of variable storage)..
  while (msb != 255) {
    // wait for serial..
    while (not sensor.available() && tries < MAX_TRY_SERIAL) {
      delay(10);
      tries++;
    }
    if (tries == MAX_TRY_SERIAL) {
      Serial.println(" TIMED OUT WAITING FOR SERIAL.");
      return -1;
    }
    msb = sensor.read();
  }

  // now we collect MSB, LSB, Checksum..
  while (not sensor.available()) {
    delay(10);
  }
  msb = sensor.read();

  while (not sensor.available()) {
    delay(10);
  }
  lsb = sensor.read();

  while (not sensor.available()) {
    delay(10);
  }
  checksum = sensor.read();

  // is the checksum ok?
  checkcalc = 255 + msb + lsb;

  if (checksum == checkcalc) {
    distance = msb * 256 + lsb;
    // Round from mm to cm
    distance += 5;
    distance = distance / 10;

    return distance;
  } else {
    Serial.println("bad checksum - ignoring reading.");
    return -1;
  }

} // end of GetDistance()

int getdist_sermode() {

  int current_reading;
  current_reading = getDistance();

  return current_reading;
}

void loop() {
  char charBuf[50];
  

  loopCount += 1;
  if (!client.connected()) {
    maxRetries -= 1;
    reconnect();
  }
  if (maxRetries == 0) {
    Serial.println("too many disconnects, rebooting");
    ESP.restart();
  }

  int newState = getdist_sermode();
  // take the maximum distance found. other distances are probably noise
  state = max(state, newState);
  if (loopCount % 60 == 0) {
    Serial.print("got new state ");
    Serial.print(state);
    Serial.println("..");

    if ((state != oldState) && (state > 0)) {
      itoa(state, charBuf, 10);
      if (!client.publish(STATE_TOPIC, charBuf)) {
        maxRetries -= 1;
      }
      oldState = state;
    }
    state = -1;
  }
  if (loopCount % 600 == 0) {
    ltoa(loopCount, charBuf, 10);
    if (!client.publish(ONLINE_TOPIC, charBuf, true)) {
      maxRetries -= 1;
    }
  }
  client.loop();
}

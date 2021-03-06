#include "wifi_mgmt.h"
WiFiClient wifi_client;
char mqtt_server[40];
char mqtt_port[6] = "1883";
char mqtt_user[60];
char mqtt_pass[60];
char identity[60] = "";

WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server,
                                        40);
WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 5);
WiFiManagerParameter custom_mqtt_user("username", "mqtt username", mqtt_user,
                                      60);
WiFiManagerParameter custom_mqtt_pass("password", "mqtt password", "",
                                      60); // dont leak the password

// flag for saving data
bool shouldSaveConfig = false;

// callback notifying us of the need to save config
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

bool save_littlefs_config() {
  DynamicJsonBuffer jsonBuffer;
  JsonObject &json = jsonBuffer.createObject();
  json["mqtt_server"] = mqtt_server;
  json["mqtt_port"] = mqtt_port;
  json["mqtt_user"] = mqtt_user;
  json["mqtt_pass"] = mqtt_pass;
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
    return false;
  } else {
    json.prettyPrintTo(Serial);
    json.printTo(configFile);
    configFile.close();
    Serial.println("saved config file");
    json.prettyPrintTo(Serial);
    return true;
  }
}

void blink(int n_times, int dlay
) {
  for (int i = 0; i < n_times; i++) {
    digitalWrite(GPIO2, 0);
    delay(dlay);
    digitalWrite(GPIO2, 1);
    delay(dlay);
  }
}


void check_reset_pin() {
  if (!digitalRead(GPIO0)) {
    delay(100);
    Serial.println("GPIO0 pin pulled down during operation! Factory reset!");
    blink(10,200);
    clear_mqtt_settings();
    WiFi.disconnect(true);
    ESP.restart();
  }

}

std::shared_ptr<PubSubClient> setup_wifi_and_mqtt(MQTT_CALLBACK_SIGNATURE) {
  bool has_config = read_littlefs_config();
  auto wm = createWifiManager();
  if (!has_config) {
    wm->resetSettings();
  }
  wm->setConfigPortalTimeout(180);
  Serial.println(String("connecting as /") + String(ESP.getChipId()));
  if (!wm->autoConnect(String(ESP.getChipId()).c_str(), "admin")) {
    Serial.println("Connection failed, resetting");
    ESP.restart();
    return nullptr;
  }
  Serial.println("autoconf done");
  int internal_retries = 20;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    check_reset_pin();
    internal_retries -= 1;
    if (internal_retries < 1) {
      Serial.println("wifi fail, rebooting");
      ESP.restart();
    }
    Serial.print(".");
  }

  if (shouldSaveConfig) {
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(mqtt_user, custom_mqtt_user.getValue());
    strcpy(mqtt_pass, custom_mqtt_pass.getValue());
    save_littlefs_config();
  }

  std::shared_ptr<PubSubClient> mqtt = std::make_shared<PubSubClient>();
  
  mqtt->setClient(wifi_client);
  mqtt->setServer(mqtt_server, atoi(mqtt_port));
  mqtt->setCallback(callback);
  auto client_id = String(ESP.getChipId());
  auto sonline_topic = online_topic();
  Serial.println(String("connecting to MQTT broker:") +  mqtt_server + " / " + mqtt_port + " / "  + mqtt_user + " / " + mqtt_pass + " / " + client_id + " / " + sonline_topic);
  if (!mqtt->connect(client_id.c_str(), 
                    strlen(mqtt_user) == 0 ? NULL : mqtt_user, 
                    strlen(mqtt_pass) == 0 ? NULL : mqtt_pass,
                    sonline_topic.c_str(), 0, 1, "0")) {
    Serial.println("MQTT connection failed. restarting ...");
    check_reset_pin();
    ESP.restart();
  }
  Serial.println("connection fine");
  return mqtt;
}

std::shared_ptr<WiFiManager> createWifiManager() {
  std::shared_ptr<WiFiManager> manager = std::make_shared<WiFiManager>();
  manager->setSaveConfigCallback(saveConfigCallback);
  manager->addParameter(&custom_mqtt_server);
  manager->addParameter(&custom_mqtt_port);
  manager->addParameter(&custom_mqtt_user);
  manager->addParameter(&custom_mqtt_pass);
  return manager;
}

void clear_mqtt_settings() {
    if (!LittleFS.begin()) {
        return;
    }
    LittleFS.remove("/config.json");
}

bool read_littlefs_config() {
  if (!LittleFS.begin()) {
    return false;
  }
  if (!LittleFS.exists("/config.json")) {
    return false;
  }
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    return false;
  }
  size_t size = configFile.size();
  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);
  DynamicJsonBuffer jsonBuffer;
  JsonObject &json = jsonBuffer.parseObject(buf.get());
  if (!json.success()) {
    return false;
  }
  strcpy(mqtt_server, json["mqtt_server"]);
  strcpy(mqtt_port, json["mqtt_port"]);
  strcpy(mqtt_user, json["mqtt_user"]);
  strcpy(mqtt_pass, json["mqtt_pass"]);
  return true;
}
String unique_topic(const char *topic) {
  String ONLINE_TOPIC = "/";
  ONLINE_TOPIC += String(ESP.getChipId());
  ONLINE_TOPIC += "/";
  ONLINE_TOPIC += topic;
  return ONLINE_TOPIC;
}

String online_topic() { return unique_topic("ONLINE"); }
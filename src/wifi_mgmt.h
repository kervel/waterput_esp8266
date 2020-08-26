#ifndef WIFIMGMT_H
#define WIFIMGMT_H
#include <LittleFS.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>            
#include <ESP8266WebServer.h>     
#include <WiFiManager.h>          
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <tuple>


std::shared_ptr<PubSubClient> setup_wifi_and_mqtt(MQTT_CALLBACK_SIGNATURE);
std::shared_ptr<WiFiManager> createWifiManager();
bool read_littlefs_config();
bool save_littlefs_config();
String unique_topic(const char * topic);
String online_topic();
void clear_mqtt_settings();
#endif
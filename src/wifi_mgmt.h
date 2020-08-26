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

char mqtt_server[40];
char mqtt_port[6] = "8080";
char mqtt_user[60];
char mqtt_pass[60];
char identity[60] = "";

std::shared_ptr<PubSubClient> setup_wifi_and_mqtt(MQTT_CALLBACK_SIGNATURE);
std::shared_ptr<WiFiManager> createWifiManager();
bool read_littlefs_config();
bool save_littlefs_config();
String unique_topic(char * topic);
String online_topic();
#endif
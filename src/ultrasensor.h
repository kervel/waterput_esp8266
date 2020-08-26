#ifndef ULTRASENSOR_H
#define ULTRASENSOR_H
#include <SoftwareSerial.h>
const int pingPin = D6; // Trigger Pin of Ultrasonic Sensor GPIO12
const int echoPin = D7; // Echo Pin of Ultrasonic Sensor GPIO13

SoftwareSerial sensor; // RX = echoPin
const int MAX_TRY_SERIAL = 50;

void setup_sermode();
void setup_triggermode();

double getdist_sermode();
double getdist_triggermode();

#endif
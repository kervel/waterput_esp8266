#include "ultrasensor.h"
SoftwareSerial sensor; // RX = echoPin

void setup_sermode() {
  sensor.begin(9600,SWSERIAL_8N1, echoPin, pingPin, false, 16536, 4096);
}

void setup_triggermode() {
  pinMode(pingPin, OUTPUT);
  pinMode(echoPin, INPUT);
  Serial.begin(9600);
}

double microsecondsToCentimeters(long microseconds) {
  return ((double)(microseconds)) / 29.0 / 2.0;
}


double getdist_triggermode() {
  long duration, duration2;
  double cm;
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(pingPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  //duration2 = pulseIn(echoPin, LOW);
  if (duration > 50000) {
      return 0;
  }
  cm = microsecondsToCentimeters(duration);
  //Serial.print("microsecs: ");
  //Serial.println(duration);
  //Serial.println(duration2);
  delay(100);
  return cm;
}


// source:
// http://www.setfirelabs.com/building-automation/dyp-me007y-tx-serial-output-ultrasonic-sensor-interfacing-with-arduino-nodemcu
double getdist_sermode() {
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

#pragma once

#include <Arduino.h>

class UltrasonicSensor {
 public:
  UltrasonicSensor(uint8_t trigPin, uint8_t echoPin);
  void begin();
  // Devuelve false ante timeout o una lectura fuera del rango de trabajo.
  bool readCentimeters(float &distanceCm);

 private:
  uint8_t trigPin_;
  uint8_t echoPin_;
};

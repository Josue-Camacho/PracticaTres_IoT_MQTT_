#pragma once
#include <Arduino.h>
#include "MqttConnection.h"
#include "ServoController.h"
#include "UltrasonicSensor.h"

class Application {
 public:
  Application();
  void begin();
  void loop();
 private:
  static void handleCommand(const uint8_t *payload, unsigned int length);
  void processCommand(const uint8_t *payload, unsigned int length);
  static Application *instance_;
  UltrasonicSensor sensor_;
  ServoController servo_;
  MqttConnection mqtt_;
  unsigned long lastReadingMs_ = 0;
};

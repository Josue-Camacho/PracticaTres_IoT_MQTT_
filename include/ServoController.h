#pragma once

#include <Arduino.h>
#include <atomic>

class ServoController {
 public:
  ServoController(uint8_t pin, int minAngle, int maxAngle, int minPulseUs,
                  int maxPulseUs);
  bool ready() const;
  bool setTarget(int angle);
  void update();

 private:
  uint8_t pin_;
  int minAngle_;
  int maxAngle_;
  int minPulseUs_;
  int maxPulseUs_;
  bool attached_ = false;
  std::atomic<int> target_{-1};
  int currentAngle_ = -1;
  unsigned long lastStepMs_ = 0;
  bool writeAngle(int angle);
};

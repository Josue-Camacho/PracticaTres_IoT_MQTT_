#include "UltrasonicSensor.h"

namespace {
constexpr unsigned long kEchoTimeoutUs = 25000;
constexpr float kMinCm = 10.0f;
constexpr float kMaxCm = 100.0f;
}

UltrasonicSensor::UltrasonicSensor(uint8_t trigPin, uint8_t echoPin)
    : trigPin_(trigPin), echoPin_(echoPin) {}

void UltrasonicSensor::begin() {
  pinMode(trigPin_, OUTPUT);
  digitalWrite(trigPin_, LOW);
  pinMode(echoPin_, INPUT);
}

bool UltrasonicSensor::readCentimeters(float &distanceCm) {
  digitalWrite(trigPin_, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin_, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin_, LOW);
  const unsigned long duration = pulseIn(echoPin_, HIGH, kEchoTimeoutUs);
  if (duration == 0) return false;
  const float measured = duration * 0.0343f / 2.0f;
  if (measured < kMinCm || measured > kMaxCm) return false;
  distanceCm = measured;
  return true;
}

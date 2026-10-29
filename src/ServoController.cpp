#include "ServoController.h"

namespace {
constexpr uint8_t kChannel = 0;
constexpr uint8_t kResolutionBits = 16;
constexpr uint32_t kPeriodUs = 20000;
constexpr uint32_t kMaxDuty = (1UL << kResolutionBits) - 1;
}

ServoController::ServoController(uint8_t pin, int minAngle, int maxAngle,
                                 int minPulseUs, int maxPulseUs)
    : pin_(pin), minAngle_(minAngle), maxAngle_(maxAngle),
      minPulseUs_(minPulseUs), maxPulseUs_(maxPulseUs) {}

bool ServoController::ready() const {
  return minAngle_ >= 0 && maxAngle_ <= 180 &&
         minAngle_ < maxAngle_ && minPulseUs_ >= 500 &&
         maxPulseUs_ <= 2500 && minPulseUs_ < maxPulseUs_;
}

bool ServoController::setTarget(int angle) {
  if (!ready() || angle < minAngle_ || angle > maxAngle_) return false;
  target_.store(angle);
  return true;
}

void ServoController::update() {
  const int target = target_.load();
  if (target < 0) return;
  const unsigned long now = millis();
  if (currentAngle_ < 0) {
    if (writeAngle(target)) {
      currentAngle_ = target;
      lastStepMs_ = now;
    }
    return;
  }
  if (currentAngle_ == target || now - lastStepMs_ < 20) return;
  const int next = currentAngle_ + (target > currentAngle_ ? 1 : -1);
  if (writeAngle(next)) currentAngle_ = next;
  lastStepMs_ = now;
}

bool ServoController::writeAngle(int angle) {
  const uint32_t pulseUs = minPulseUs_ +
      static_cast<uint32_t>(angle - minAngle_) * (maxPulseUs_ - minPulseUs_) /
          (maxAngle_ - minAngle_);
  const uint32_t duty = (pulseUs * kMaxDuty) / kPeriodUs;
  if (!attached_) {
    if (ledcSetup(kChannel, 50, kResolutionBits) == 0) return false;
    ledcWrite(kChannel, duty);
    ledcAttachPin(pin_, kChannel);
    attached_ = true;
  } else {
    ledcWrite(kChannel, duty);
  }
  return true;
}

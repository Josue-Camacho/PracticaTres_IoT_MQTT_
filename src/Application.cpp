#include "Application.h"
#include "Config.h"

Application *Application::instance_ = nullptr;

Application::Application()
    : sensor_(ULTRASONIC_TRIG_GPIO, ULTRASONIC_ECHO_GPIO),
      servo_(SERVO_GPIO, SERVO_MIN_DEGREES, SERVO_MAX_DEGREES,
             SERVO_MIN_PULSE_US, SERVO_MAX_PULSE_US),
      mqtt_(handleCommand) {}

void Application::begin() {
  Serial.begin(115200);
  instance_ = this;
  sensor_.begin();
  Serial.println(servo_.ready() ? "Servo preparado; sin movimiento inicial." : "Servo deshabilitado.");
  mqtt_.begin();
}

void Application::loop() {
  servo_.update();
  const unsigned long now = millis();
  if (now - lastReadingMs_ >= 500) {
    lastReadingMs_ = now;
    float distance;
    if (sensor_.readCentimeters(distance)) {
      mqtt_.publishDistance(distance);
    } else {
      Serial.println("Distancia invalida: sin eco o fuera de 10-100 cm.");
    }
  }
  delay(1);
}

void Application::handleCommand(const uint8_t *payload, unsigned int length) {
  if (instance_) instance_->processCommand(payload, length);
}

void Application::processCommand(const uint8_t *payload, unsigned int length) {
  if (!length || length > 3) { Serial.println("MQTT: comando invalido."); return; }
  int angle = 0;
  for (unsigned int i = 0; i < length; ++i) {
    if (payload[i] < '0' || payload[i] > '9') { Serial.println("MQTT: comando invalido."); return; }
    angle = angle * 10 + (payload[i] - '0');
  }
  if (angle > 180 || angle < SERVO_MIN_DEGREES || angle > SERVO_MAX_DEGREES) {
    Serial.println("MQTT: angulo fuera de limites."); return;
  }
  if (!servo_.ready()) { Serial.println("MQTT: servo deshabilitado."); return; }
  if (!servo_.setTarget(angle)) { Serial.println("MQTT: comando rechazado."); return; }
  Serial.printf("MQTT: posicion solicitada %d grados.\n", angle);
}

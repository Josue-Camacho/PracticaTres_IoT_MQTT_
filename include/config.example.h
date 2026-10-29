#pragma once

// Copiar a config.local.h y completar localmente en una etapa posterior.
// Se usa como alternativa cuando no existe config.local.h.
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define MQTT_HOST "broker.hivemq.com"
#define MQTT_PORT 1883
#define MQTT_TOPIC_PREFIX "sis234/grupo3"

// Provisionales: verificar el hardware y las conexiones antes de usarlos.
#define ULTRASONIC_TRIG_GPIO 18
#define ULTRASONIC_ECHO_GPIO 19
#define SERVO_GPIO 25
#define SERVO_CLOSED_DEGREES 0
#define SERVO_OPEN_DEGREES 90
#define SERVO_MIN_DEGREES 0
#define SERVO_MAX_DEGREES 180

// Pulsos nominales ajustables; comprobar compatibilidad con el servo y mecanismo.
#define SERVO_MIN_PULSE_US 500
#define SERVO_MAX_PULSE_US 2400

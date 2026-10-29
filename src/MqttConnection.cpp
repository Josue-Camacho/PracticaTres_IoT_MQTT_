#include "MqttConnection.h"
#include "Config.h"
#include <WiFi.h>

#include <string.h>

namespace {
constexpr unsigned long kRetryMs = 5000;

}

void RetainAwareClient::reset() {
  remaining_ = 0; multiplier_ = 1; phase_ = 0; retained_ = false;
}
int RetainAwareClient::connect(IPAddress ip, uint16_t port) {
  reset();
  return client_.connect(ip, port);
}
int RetainAwareClient::connect(const char *host, uint16_t port) {
  reset();
  return client_.connect(host, port);
}
void RetainAwareClient::observe(uint8_t b) {
  if (phase_ == 0) {
    retained_ = ((b & 0xf0) == 0x30) && (b & 1);
    remaining_ = 0; multiplier_ = 1; phase_ = 1;
  } else if (phase_ == 1) {
    remaining_ += (b & 0x7f) * multiplier_;
    if (b & 0x80) multiplier_ *= 128;
    else phase_ = remaining_ ? 2 : 0;
  } else if (--remaining_ == 0) {
    phase_ = 0;
  }
}
int RetainAwareClient::read() {
  const int b = client_.read();
  if (b >= 0) observe(static_cast<uint8_t>(b));
  return b;
}
int RetainAwareClient::read(uint8_t *b, size_t n) {
  const int count = client_.read(b, n);
  for (int i = 0; i < count; ++i) observe(b[i]);
  return count;
}

MqttConnection::MqttConnection(CommandHandler handler)
    : handler_(handler), transport_(tcp_), mqtt_(transport_) {}

bool MqttConnection::begin() {
  const char *prefix = MQTT_TOPIC_PREFIX;
  if (!WIFI_SSID[0] || !MQTT_HOST[0] || !prefix[0] ||
      strstr(prefix, "REEMPLAZAR") || strpbrk(prefix, "+#") ||
      prefix[strlen(prefix) - 1] == '/') {
    Serial.println("WiFi/MQTT deshabilitado: complete SSID y prefijo en config.local.h.");
    return false;
  }
  if (snprintf(publishTopic_, sizeof(publishTopic_), "%s/distancia/cm", prefix) >= sizeof(publishTopic_) ||
      snprintf(commandTopic_, sizeof(commandTopic_), "%s/servo/cmd", prefix) >= sizeof(commandTopic_)) {
    Serial.println("MQTT: prefijo demasiado largo.");
    return false;
  }
  distances_ = xQueueCreate(1, sizeof(float));
  if (!distances_) return false;
  mqtt_.setServer(MQTT_HOST, MQTT_PORT);
  mqtt_.setSocketTimeout(5);
  mqtt_.setCallback([this](char *topic, uint8_t *payload, unsigned int length) {
    onMessage(topic, payload, length);
  });
  if (xTaskCreate(taskEntry, "mqtt", 12288, this, 1, nullptr) != pdPASS) {
    vQueueDelete(distances_); distances_ = nullptr;
    Serial.println("MQTT: no se pudo crear la tarea de red.");
    return false;
  }
  return true;
}

void MqttConnection::publishDistance(float cm) {
  if (distances_) xQueueOverwrite(distances_, &cm);
}
void MqttConnection::taskEntry(void *context) {
  static_cast<MqttConnection *>(context)->run();
}
void MqttConnection::onMessage(char *topic, uint8_t *payload, unsigned int length) {
  if (strcmp(topic, commandTopic_) != 0) return;
  Serial.printf("MQTT: mensaje en %s: ", topic);
  Serial.write(payload, length);
  Serial.println();
  if (transport_.retained()) {
    Serial.println("MQTT: comando retenido descartado.");
    return;
  }
  handler_(payload, length);
}
void MqttConnection::run() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("WiFi: conectando.");
  bool hadWifi = false;
  bool hadMqtt = false;


  unsigned long lastWifiAttempt = millis();
  unsigned long lastMqttAttempt = millis() - kRetryMs;
  for (;;) {
    const unsigned long now = millis();
    if (hadMqtt && !mqtt_.connected()) {
      Serial.println("MQTT: desconectado.");
      hadMqtt = false;
    }
    if (WiFi.status() != WL_CONNECTED) {
      xQueueReset(distances_);
      if (hadWifi) {
        Serial.println("WiFi: desconectado.");
        mqtt_.disconnect();
        hadWifi = false;
      }
      if (now - lastWifiAttempt >= kRetryMs) {
        lastWifiAttempt = now;
        WiFi.reconnect();
      }
    } else {
      if (!hadWifi) {
        Serial.println("WiFi: conectado.");
        hadWifi = true;
      }
      if (!mqtt_.connected() && now - lastMqttAttempt >= kRetryMs) {
        lastMqttAttempt = now;
        char id[32];
        const uint64_t mac = ESP.getEfuseMac();
        snprintf(id, sizeof(id), "esp32-%04X%08lX", static_cast<unsigned int>(mac >> 32), static_cast<unsigned long>(mac & 0xffffffffULL));
        Serial.println("MQTT: conectando.");
        const bool connected = mqtt_.connect(id);
        if (connected) {
          if (mqtt_.subscribe(commandTopic_, 0)) {
            hadMqtt = true;
            Serial.printf("MQTT: conectado; suscripcion solicitada: %s.\n", commandTopic_);
          }
          else { Serial.println("MQTT: fallo al suscribir."); mqtt_.disconnect(); }
        } else Serial.printf("MQTT: conexion fallida (%d).\n", mqtt_.state());
      }
      if (mqtt_.connected()) {
        mqtt_.loop();
        float distance;
        if (xQueueReceive(distances_, &distance, 0) == pdTRUE) {
          char value[16];
          snprintf(value, sizeof(value), "%.1f", distance);
          if (mqtt_.publish(publishTopic_, value, false)) Serial.printf("MQTT: publicado en %s: %s\n", publishTopic_, value);
          else Serial.println("MQTT: fallo al publicar.");
        }
      } else {
        // Nunca publicar una medicion encolada antes de la reconexion.
        xQueueReset(distances_);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

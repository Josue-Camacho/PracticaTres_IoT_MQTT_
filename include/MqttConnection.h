#pragma once

#include <Arduino.h>
#include <Client.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Observa el encabezado MQTT sin alterar los bytes entregados a PubSubClient.
class RetainAwareClient : public Client {
 public:
  explicit RetainAwareClient(WiFiClient &client) : client_(client) {}
  int connect(IPAddress ip, uint16_t port) override;
  int connect(const char *host, uint16_t port) override;
  size_t write(uint8_t b) override { return client_.write(b); }
  size_t write(const uint8_t *b, size_t n) override { return client_.write(b, n); }
  int available() override { return client_.available(); }
  int read() override;
  int read(uint8_t *b, size_t n) override;
  int peek() override { return client_.peek(); }
  void flush() override { client_.flush(); }
  void stop() override { client_.stop(); }
  uint8_t connected() override { return client_.connected(); }
  operator bool() override { return connected(); }
  bool retained() const { return retained_; }
 private:
  void reset();
  void observe(uint8_t b);
  WiFiClient &client_;
  uint32_t remaining_ = 0;
  uint32_t multiplier_ = 1;
  uint8_t phase_ = 0;
  bool retained_ = false;
};

class MqttConnection {
 public:
  using CommandHandler = void (*)(const uint8_t *, unsigned int);
  explicit MqttConnection(CommandHandler handler);
  bool begin();
  void publishDistance(float cm);
 private:
  static void taskEntry(void *context);
  void run();
  void onMessage(char *topic, uint8_t *payload, unsigned int length);
  CommandHandler handler_;
  WiFiClient tcp_;
  RetainAwareClient transport_;
  PubSubClient mqtt_;
  QueueHandle_t distances_ = nullptr;
  char publishTopic_[160] = {};
  char commandTopic_[160] = {};
};

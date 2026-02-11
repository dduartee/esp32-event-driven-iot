#include "iot_mqtt.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "event_bits.h"

static const char* TAG = "mqtt";


void mqtt_event_handler(void* arg, esp_event_base_t event_base,
                        int32_t event_id, void* event_data) {
  esp_mqtt_event_handle_t event = event_data;
  switch (event_id) {
    case MQTT_EVENT_CONNECTED:
      ESP_LOGI(TAG, "MQTT conectado");
      esp_mqtt_client_subscribe(event->client, "hello_world/test", 0);
      esp_mqtt_client_publish(event->client, "hello_world/test", "ola do esp32c3", 0, 0, 0);
      break;
    case MQTT_EVENT_DATA:
      ESP_LOGI(TAG, "Topico: %.*s", event->topic_len, event->topic);
      ESP_LOGI(TAG, "Dados: %.*s", event->data_len, event->data);
      break;
    case MQTT_EVENT_DISCONNECTED:
      ESP_LOGI(TAG, "MQTT desconectado");
      break;
  }
}

void mqtt_init() {
    esp_mqtt_client_config_t mqtt_cfg = {
    .broker.address.uri = "mqtt://broker.hivemq.com:1883",
  };

  esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
  esp_mqtt_client_start(client);
}

void mqtt_task(void* arg) {
  // bloqueia aqui até o bit ser setado (não consome CPU)
  xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT,
                      false,    // não limpa o bit após ler
                      true,     // espera todos os bits
                      portMAX_DELAY);  // espera pra sempre

  ESP_LOGI(TAG, "WiFi conectado, iniciando MQTT...");
  mqtt_init();
  vTaskDelete(NULL);  // task termina após iniciar o MQTT
}

#include "iot_wifi.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "event_bits.h"
#include "freertos/event_groups.h"
#include "sdkconfig.h"

static const char* TAG = "wifi";
void wifi_event_handler(void* arg, esp_event_base_t event_base,
                        int32_t event_id, void* event_data) {
  char TAG_WIFI_EVENT[] = "wifi_event_handler";
  ESP_LOGI(TAG_WIFI_EVENT,
           "Event dispatched from event loop base=%s, event_id=%d", event_base,
           event_id);
  switch (event_id) {
    case WIFI_EVENT_STA_START:
      esp_wifi_connect();
      break;
    case WIFI_EVENT_STA_CONNECTED:
      ESP_LOGI(TAG_WIFI_EVENT, "connected to ap SSID:%s",
               ((wifi_event_sta_connected_t*)event_data)->ssid);
      break;
    case WIFI_EVENT_STA_DISCONNECTED:
      ESP_LOGI(TAG_WIFI_EVENT, "connect to the AP fail");
      vTaskDelay(pdMS_TO_TICKS(5000));
      esp_wifi_connect();
      ESP_LOGW(TAG_WIFI_EVENT, "retry to connect to the AP");
      break;

    default:
      break;
  }
}

void got_ip_handler(void* arg, esp_event_base_t event_base, int32_t event_id,
                    void* event_data) {
  ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
  ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
  xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
}
void wifi_init() {
  esp_netif_init();  // inicializa o TCP/IP stack
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();  // cria uma interface de rede padrão
                                        // para o wifi station.
  esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler,
                             NULL);
  esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &got_ip_handler,
                             NULL);

  wifi_config_t wifi_config = {
      .sta =
          {
              .ssid = CONFIG_WIFI_SSID,
              .password = CONFIG_WIFI_PASSWORD,
              /* Setting a password implies station will connect to all security
               * modes including WEP/WPA. However these modes are deprecated and
               * not advisable to be used. Incase your Access point doesn't
               * support WPA2, these mode can be enabled by commenting below
               * line */
              .threshold.authmode = WIFI_AUTH_WPA2_PSK,

              .pmf_cfg = {.capable = true, .required = false},
          },
  };
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
}

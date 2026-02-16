#include <inttypes.h>
#include <stdio.h>

#include "esp_chip_info.h"
#include "esp_event.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "iot_mqtt.h"
#include "iot_ota.h"
#include "iot_wifi.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "event_bits.h"

// Version header, auto-gerado pelo CI/CD (vide .gitignore)
#ifdef __has_include
  #if __has_include("version.h")
    #include "version.h"
  #endif
#endif

EventGroupHandle_t wifi_event_group;
static const char* TAG = "app_main";

void print_esp_info() {
  /* Print chip information */
  esp_chip_info_t chip_info;
  uint32_t flash_size;
  esp_chip_info(&chip_info);
  ESP_LOGI(TAG, "This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET, chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154)
               ? ", 802.15.4 (Zigbee/Thread)"
               : "");

  unsigned major_rev = chip_info.revision / 100;
  unsigned minor_rev = chip_info.revision % 100;
  ESP_LOGI(TAG, "silicon revision v%d.%d, ", major_rev, minor_rev);
  if (esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
    ESP_LOGI(TAG, "Get flash size failed");
    return;
  }

  ESP_LOGI(
      TAG, "%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
      (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

  ESP_LOGI(TAG, "Minimum free heap size: %" PRIu32 " bytes\n",
           esp_get_minimum_free_heap_size());
}

void app_main(void) {
  print_esp_info();

  // Log da partição atual em execução
  const esp_partition_t *running = esp_ota_get_running_partition();
  ESP_LOGI(TAG, "Running partition: %s", running->label);

#ifdef APP_VERSION_STRING
  ESP_LOGI(TAG, "Firmware version: %s", APP_VERSION_STRING);
#else
  ESP_LOGI(TAG, "Firmware version: development (not from release)");
#endif

  ESP_LOGI(TAG, "Hello world!\n");

  wifi_event_group = xEventGroupCreate();

  nvs_flash_init();
  wifi_init();

  xTaskCreate(mqtt_task, "mqtt_task", 4096, NULL, 5, NULL);
  xTaskCreate(ota_task, "ota_task", 8192, NULL, 4, NULL);

  ESP_LOGI(TAG, "app_main finished.");
}

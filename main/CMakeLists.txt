idf_component_register(SRCS "iot_ota.c" "main.c" "iot_wifi.c" "iot_mqtt.c"
                       PRIV_REQUIRES mqtt esp_wifi esp_event nvs_flash spi_flash esp_https_ota app_update esp_http_client
                       INCLUDE_DIRS "")

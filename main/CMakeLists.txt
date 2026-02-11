idf_component_register(SRCS "main.c" "iot_wifi.c" "iot_mqtt.c"
                       PRIV_REQUIRES mqtt esp_wifi esp_event nvs_flash spi_flash
                       INCLUDE_DIRS "")

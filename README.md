# esp32-event-driven-iot

Base event-driven para IoT em ESP32-C3. WiFi station + MQTT client desacoplados via FreeRTOS Event Groups.

## Arquitetura

```
app_main()
  ├─ xEventGroupCreate()
  ├─ wifi_init()
  ├─ xTaskCreate(mqtt_task)
  └─ retorna (FreeRTOS continua)
       │
       ├─ WiFi: GOT_IP → xEventGroupSetBits(WIFI_CONNECTED_BIT)
       │                         │
       └─ mqtt_task: WaitBits ───┘→ mqtt_init() → subscribe + publish
```

| Arquivo | Responsabilidade |
|---|---|
| `main.c` | Entry point, cria event group, orquestra init |
| `iot_wifi.c/h` | WiFi STA, sinaliza `WIFI_CONNECTED_BIT` ao obter IP |
| `iot_mqtt.c/h` | MQTT client (HiveMQ), aguarda WiFi via Event Group |
| `event_bits.h` | Bits compartilhados e `extern` do event group |

## Build

```bash
idf.py build              # compilar
idf.py flash monitor      # gravar e monitorar
idf.py menuconfig         # configuracao interativa
```

Requer ESP-IDF com `$IDF_PATH` configurado.

## Proximos Passos

- [X] Reconexao WiFi automatica com backoff exponencial
- [X] Credenciais WiFi via Kconfig (`menuconfig`) ao inves de hardcoded
- [ ] OTA updates (`esp_https_ota`) com rollback automatico
- [ ] Tasks dedicadas para leitura de sensores
- [ ] NVS para armazenamento persistente de configuracoes
- [ ] Migrar modulos para `components/` quando o projeto crescer

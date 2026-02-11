# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Communication Style

- Responda sempre em **português brasileiro**
- O dev vem do Arduino/PlatformIO e está aprendendo ESP-IDF — sempre que possível, faça paralelos com o mundo Arduino (ex: `WiFi.begin()` → `esp_wifi_start()` + `esp_wifi_connect()`)
- Use tabelas comparativas (Arduino vs ESP-IDF) para explicar equivalências
- Use diagramas de fluxo em texto (com `└─>`, `├─`, setas) para mostrar sequências de eventos e callbacks
- Explique o **porquê** das coisas, não só o como — o dev quer entender a arquitetura, não só copiar código
- Seja direto e conciso, mas sem sacrificar a clareza. Use marcadores visuais (checkmarks, X) para status
- Quando revisar código, liste problemas numerados com explicação curta e mostre a sequência correta visualmente

## Project Overview

ESP-IDF project targeting ESP32-C3 (RISC-V, single core). WiFi station + MQTT client with event-driven architecture using FreeRTOS. Build uses `MINIMAL_BUILD` mode.

## Build Commands

```bash
idf.py build              # Build the project
idf.py flash monitor      # Flash to device and open serial monitor
idf.py monitor            # Serial monitor only (Ctrl+] to exit)
idf.py menuconfig         # Interactive configuration (sdkconfig)
idf.py fullclean          # Clean build artifacts completely
```

Requires ESP-IDF environment sourced (`$IDF_PATH` set).

## Architecture

### Module Structure

- `main.c` — Entry point (`app_main`), creates event group, initializes WiFi, spawns MQTT task
- `iot_wifi.c/h` — WiFi STA init and event handlers. Signals `WIFI_CONNECTED_BIT` on IP acquisition
- `iot_mqtt.c/h` — MQTT client (HiveMQ public broker). Waits for `WIFI_CONNECTED_BIT` before connecting
- `event_bits.h` — Shared FreeRTOS Event Group bits and `extern` declaration for `wifi_event_group`

### Inter-module Communication

Modules are decoupled via FreeRTOS Event Groups — WiFi does not call MQTT directly:

```
app_main()
  ├─ xEventGroupCreate()
  ├─ wifi_init()
  ├─ xTaskCreate(mqtt_task)
  └─ returns (FreeRTOS keeps running)
       │
       ├─ WiFi: GOT_IP → xEventGroupSetBits(WIFI_CONNECTED_BIT)
       │                         │
       └─ mqtt_task: WaitBits ───┘→ mqtt_init() → subscribe + publish
```

### Adding New Source Files

1. Create `new_module.c` and `new_module.h` in `main/`
2. Add `"new_module.c"` to `SRCS` in `main/CMakeLists.txt`
3. Add any new ESP-IDF components to `PRIV_REQUIRES`

### Component Dependencies

Declared in `main/CMakeLists.txt` via `PRIV_REQUIRES`: `mqtt`, `esp_wifi`, `esp_event`, `nvs_flash`, `spi_flash`.

## Conventions

- Use `ESP_LOGI`/`ESP_LOGW`/`ESP_LOGE` for logging (not `printf`)
- Wrap ESP-IDF API calls with `ESP_ERROR_CHECK()` to catch failures early
- Language: C (not C++)
- Each `.c` includes only what it directly uses — no reliance on transitive includes
- Shared global state uses `extern` in a header with definition in a single `.c`
- WiFi credentials are configured via Kconfig (`idf.py menuconfig` → WiFi Configuration)

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

## CI/CD e Releases

Projeto usa **GitHub Actions** para automação:

### Build Automático (CI)

A cada push em qualquer branch, o workflow `.github/workflows/ci.yml`:
- ✓ Compila o firmware automaticamente
- ✓ Valida que a compilação é bem-sucedida
- ✓ Upload de artifacts (binários) para inspeção

Ver [GitHub Actions](https://github.com/dduartee/esp32-event-driven-iot/actions) para logs de compilação.

### Releases com Versionamento

Para criar uma **release**, use semantic versioning com git tags:

```bash
# 1. Tag com versão (ex: v0.1.0)
git tag v0.1.0

# 2. Push do tag para GitHub
git push origin v0.1.0
```

O workflow `.github/workflows/release.yml` automaticamente:
- Extrai versão do tag (v0.1.0 → 1.0.0)
- Gera `main/version.h` com macros de versão
- Compila firmware com versão embedada
- Cria GitHub Release com binários versionados:
  - `firmware-v0.1.0.bin` — Main firmware
  - `bootloader-v0.1.0.bin` — Bootloader ESP32-C3
  - `partition-table-v0.1.0.bin` — Partition table
  - `flasher_args-v0.1.0.json` — Flash arguments

Binários estão disponíveis em [GitHub Releases](https://github.com/dduartee/esp32-event-driven-iot/releases).

### Flash de Release

```bash
# Download binários da release (ex: v0.1.0)
# Depois:

esptool.py --chip esp32c3 write_flash \
  0x0 bootloader-v0.1.0.bin \
  0x8000 partition-table-v0.1.0.bin \
  0x10000 firmware-v0.1.0.bin
```

Ou usando flasher_args.json:
```bash
esptool.py --chip esp32c3 write_flash @flasher_args-v0.1.0.json
```

### Versionamento no Firmware

A versão é embedada no firmware via `APP_VERSION_STRING`. No serial monitor:

```
I (580) app_main: Firmware version: 0.1.0
```

Versão também é acessível no código via macros:
- `APP_VERSION_MAJOR` — Ex: 0
- `APP_VERSION_MINOR` — Ex: 1
- `APP_VERSION_PATCH` — Ex: 0
- `APP_VERSION_STRING` — Ex: "0.1.0"

Preparação para **OTA updates** futuro (esp_https_ota) com versões estáveis.

## Proximos Passos

- [X] Reconexao WiFi automatica com backoff exponencial
- [X] Credenciais WiFi via Kconfig (`menuconfig`) ao inves de hardcoded
- [ ] OTA updates (`esp_https_ota`) com rollback automatico
- [ ] Tasks dedicadas para leitura de sensores
- [ ] NVS para armazenamento persistente de configuracoes
- [ ] Migrar modulos para `components/` quando o projeto crescer

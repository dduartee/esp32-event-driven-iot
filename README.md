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

## OTA (Over-The-Air Updates)

Implementação completa de OTA usando **GitHub Releases** como backend.

### Como Funciona

1. **Boot do dispositivo**
   - ESP32-C3 inicia normalmente
   - Após WiFi conectar, `ota_task` dispara automaticamente

2. **Check de Updates**
   - Download de `manifest.json` em GitHub
   - Compara versão atual vs disponível

3. **Update (se necessário)**
   - Download de novo firmware em HTTPS
   - Validação via SHA256 checksum
   - Flash em partição OTA alternate
   - Reboot automático

4. **Rollback Automático**
   - Se o novo firmware falhar no boot (< 3 tentativas)
   - Dispositivo volta automaticamente para versão anterior
   - Continua funcionando normalmente

### Configuração

**Habilitar OTA via menuconfig:**
```bash
idf.py menuconfig
# OTA Configuration → OTA manifest URL (deixar padrão)
```

**URL do manifest.json:**
```
https://raw.githubusercontent.com/gabrielkduarte/esp32-event-driven-iot/main/manifest.json
```

Customizável via `CONFIG_OTA_MANIFEST_URL` em `menuconfig`.

### Partições (two_ota)

Tabela de partições automaticamente gerenciada (4 MB de flash):
```
nvs,    data, nvs,     0x9000,  16K
otadata,data, ota,     0xd000,  8K     ← Metadados de OTA
phy,    data, phy,     0xf000,  4K
factory,app,  factory, 0x10000, 1M     ← Fallback inicial
ota_0,  app,  ota_0,   0x110000, 1M    ← Partição OTA ativa
ota_1,  app,  ota_1,   0x210000, 1M    ← Partição OTA alternada
```

### CI/CD e Releases

Workflow `.github/workflows/release.yml` gera automaticamente:
1. Binários versionados (`v0.2.0`)
2. **manifest.json** com SHA256 e URL do firmware

Exemplo de release (v0.2.0):
```
esp32-event-driven-iot-v0.2.0.bin       ← Firmware
bootloader-v0.2.0.bin                   ← Bootloader
partition-table-v0.2.0.bin              ← Tabela de partições
manifest.json                           ← Metadata de OTA
```

### Fluxo de Teste

```bash
# 1. Release inicial (v0.1.0) - sem OTA ainda
git tag v0.1.0 && git push origin v0.1.0

# 2. Flashar no dispositivo
idf.py flash

# 3. Implementar OTA e fazer release v0.2.0
# (código de OTA já está aqui)
git tag v0.2.0 && git push origin v0.2.0
# → GitHub Actions compila, gera manifest.json

# 4. Release v0.3.0 (novo firmware)
git tag v0.3.0 && git push origin v0.3.0

# 5. Dispositivo (ainda em v0.2.0):
#   - Boot → checa manifest.json
#   - Vê v0.3.0 disponível
#   - Download + Flash automático
#   - Reboot → agora em v0.3.0 ✓
```

### Modulos de OTA

| Arquivo | Responsabilidade |
|---|---|
| `main/iot_ota.c/h` | Lógica de OTA (check, download, validação) |
| `scripts/generate_manifest.py` | Script para gerar manifest.json com SHA256 |
| `partitions_two_ota.csv` | Tabela de partições (ota_0 + ota_1) |

### Segurança

- ✅ **HTTPS** para download de firmware
- ✅ **SHA256 checksum** para validação de integridade
- ✅ **Rollback automático** se boot falhar
- ✅ **Versionamento semântico** para comparação de versões
- 🔒 Futuro: Assinatura digital de firmware

## Proximos Passos

- [X] Reconexao WiFi automatica com backoff exponencial
- [X] Credenciais WiFi via Kconfig (`menuconfig`) ao inves de hardcoded
- [X] OTA updates (`esp_https_ota`) com rollback automatico e GitHub Releases
- [ ] Tasks dedicadas para leitura de sensores
- [ ] NVS para armazenamento persistente de configuracoes
- [ ] Migrar modulos para `components/` quando o projeto crescer

#include "iot_ota.h"

#include <string.h>

#include "esp_app_desc.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "event_bits.h"

static const char *TAG = "ota";

// URL do manifest.json (configurável via Kconfig ou padrão)
#ifndef CONFIG_OTA_MANIFEST_URL
#define OTA_MANIFEST_URL "https://raw.githubusercontent.com/gabrielkduarte/esp32-event-driven-iot/main/manifest.json"
#else
#define OTA_MANIFEST_URL CONFIG_OTA_MANIFEST_URL
#endif

// Estrutura para metadata do manifest.json
typedef struct {
    char version[32];
    char firmware_url[256];
    char sha256[65];
    size_t size;
} ota_manifest_t;

/**
 * @brief Download e parse do manifest.json
 *
 * Faz GET request em OTA_MANIFEST_URL e extrai:
 * - version
 * - firmware_url
 * - sha256 (checksum de validação)
 * - size
 *
 * @param manifest Ponteiro para struct de manifest
 * @return ESP_OK se sucesso, ESP_FAIL caso contrário
 */
static esp_err_t ota_check_manifest(ota_manifest_t *manifest) {
    // Configuração básica do cliente HTTP (HTTPS)
    esp_http_client_config_t config = {
        .url = OTA_MANIFEST_URL,
        .timeout_ms = 10000,
        .keep_alive_enable = true,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }

    // Fazer GET request
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP GET failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return err;
    }

    int status_code = esp_http_client_get_status_code(client);
    if (status_code != 200) {
        ESP_LOGE(TAG, "HTTP response: %d (expected 200)", status_code);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    // Ler body do response (manifest.json é pequeno, max ~500 bytes)
    int content_length = esp_http_client_get_content_length(client);
    if (content_length <= 0 || content_length > 1024) {
        ESP_LOGE(TAG, "Invalid content length: %d", content_length);
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    char response_buffer[1024] = {0};
    int bytes_read = esp_http_client_read_response(client, response_buffer, sizeof(response_buffer) - 1);
    if (bytes_read <= 0) {
        ESP_LOGE(TAG, "Failed to read response");
        esp_http_client_cleanup(client);
        return ESP_FAIL;
    }

    response_buffer[bytes_read] = '\0';
    esp_http_client_cleanup(client);

    // Parse simples do JSON (sem bibliotecas externas)
    // Formato esperado:
    // {"version":"1.0.0","firmware_url":"https://...","sha256":"abc123...","size":524288}

    const char *version_start = strstr(response_buffer, "\"version\":\"");
    const char *url_start = strstr(response_buffer, "\"firmware_url\":\"");
    const char *sha256_start = strstr(response_buffer, "\"sha256\":\"");
    const char *size_start = strstr(response_buffer, "\"size\":");

    if (!version_start || !url_start || !sha256_start || !size_start) {
        ESP_LOGE(TAG, "Failed to parse manifest JSON");
        return ESP_FAIL;
    }

    // Parse version
    version_start += 11;  // skip "version":"
    sscanf(version_start, "%31[^\"]", manifest->version);

    // Parse firmware_url
    url_start += 16;  // skip "firmware_url":"
    sscanf(url_start, "%255[^\"]", manifest->firmware_url);

    // Parse sha256
    sha256_start += 10;  // skip "sha256":"
    sscanf(sha256_start, "%64[^\"]", manifest->sha256);

    // Parse size
    size_start += 7;  // skip "size":
    sscanf(size_start, "%zu", &manifest->size);

    ESP_LOGI(TAG, "Manifest parsed successfully:");
    ESP_LOGI(TAG, "  version: %s", manifest->version);
    ESP_LOGI(TAG, "  sha256: %s", manifest->sha256);
    ESP_LOGI(TAG, "  size: %zu bytes", manifest->size);

    return ESP_OK;
}

/**
 * @brief Compara versões usando semver
 *
 * @param current Versão atual (ex: "0.1.0")
 * @param available Versão disponível (ex: "1.0.0")
 * @return true se available > current, false caso contrário
 */
static bool ota_is_new_version_available(const char *current, const char *available) {
    int current_major, current_minor, current_patch;
    int available_major, available_minor, available_patch;

    // Parse versões no formato "X.Y.Z"
    if (sscanf(current, "%d.%d.%d", &current_major, &current_minor, &current_patch) != 3) {
        ESP_LOGW(TAG, "Failed to parse current version: %s", current);
        return false;
    }

    if (sscanf(available, "%d.%d.%d", &available_major, &available_minor, &available_patch) != 3) {
        ESP_LOGW(TAG, "Failed to parse available version: %s", available);
        return false;
    }

    // Comparar versões
    if (available_major != current_major) {
        return available_major > current_major;
    }
    if (available_minor != current_minor) {
        return available_minor > current_minor;
    }
    return available_patch > current_patch;
}

/**
 * @brief Download e flash do firmware usando esp_https_ota
 *
 * @param firmware_url URL de download do firmware.bin
 * @return ESP_OK se sucesso, ESP_FAIL caso contrário
 */
static esp_err_t ota_perform_update(const char *firmware_url) {
    ESP_LOGI(TAG, "Starting firmware download from: %s", firmware_url);

    esp_http_client_config_t config = {
        .url = firmware_url,
        .timeout_ms = 30000,
        .keep_alive_enable = true,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    esp_https_ota_handle_t ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_config, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA begin failed: %s", esp_err_to_name(err));
        return err;
    }

    // Download em chunks (esp_https_ota_perform faz o write na flash automaticamente)
    while (1) {
        err = esp_https_ota_perform(ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }

        // Log progresso a cada 10KB
        static int last_logged = 0;
        int bytes_read = esp_https_ota_get_image_len_read(ota_handle);
        if (bytes_read - last_logged >= 10240) {
            ESP_LOGI(TAG, "Downloaded %d bytes...", bytes_read);
            last_logged = bytes_read;
        }
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA perform failed: %s", esp_err_to_name(err));
        esp_https_ota_abort(ota_handle);
        return err;
    }

    // Finalizar OTA (valida CRC, escreve OTA state, etc)
    err = esp_https_ota_finish(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA finish failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "OTA update completed successfully");
    return ESP_OK;
}

void ota_task(void *pvParameters) {
    ESP_LOGI(TAG, "OTA task started, waiting for WiFi...");

    // Espera WiFi conectar (mesmo padrão do mqtt_task)
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);

    ESP_LOGI(TAG, "WiFi connected, checking for updates...");

    // Log da partição atual em execução
    const esp_partition_t *running = esp_ota_get_running_partition();
    ESP_LOGI(TAG, "Running partition: %s", running->label);

    // 1. Download e parse manifest.json
    ota_manifest_t manifest = {0};
    if (ota_check_manifest(&manifest) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to check manifest, skipping OTA");
        vTaskDelete(NULL);
        return;
    }

    // 2. Compara versões
    const esp_app_desc_t *app_desc = esp_app_get_description();
    ESP_LOGI(TAG, "Current version: %s", app_desc->version);
    ESP_LOGI(TAG, "Available version: %s", manifest.version);

#ifdef CONFIG_OTA_SKIP_VERSION_CHECK
    ESP_LOGI(TAG, "Version check skipped (CONFIG_OTA_SKIP_VERSION_CHECK)");
#else
    if (!ota_is_new_version_available(app_desc->version, manifest.version)) {
        ESP_LOGI(TAG, "Already up to date!");
        vTaskDelete(NULL);
        return;
    }
#endif

    // 3. Nova versão disponível, baixar e aplicar update
    ESP_LOGI(TAG, "New version available! Starting OTA update...");
    if (ota_perform_update(manifest.firmware_url) == ESP_OK) {
        ESP_LOGI(TAG, "OTA update successful, rebooting in 2 seconds...");
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA update failed");
        // Task termina, mas dispositivo continua funcionando com versão anterior
    }

    vTaskDelete(NULL);
}

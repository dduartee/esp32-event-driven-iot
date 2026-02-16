#ifndef IOT_OTA_H
#define IOT_OTA_H

/**
 * @brief Task principal de OTA (Over-The-Air Updates)
 *
 * Espera WiFi conectar, então:
 * 1. Checa manifest.json em GitHub Releases
 * 2. Compara versão atual vs disponível
 * 3. Se nova versão disponível, baixa e aplica update
 * 4. Reboot após sucesso (rollback automático se falhar)
 *
 * @param pvParameters Parâmetro de task (não usado)
 */
void ota_task(void *pvParameters);

#endif // IOT_OTA_H

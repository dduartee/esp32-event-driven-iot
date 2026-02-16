# Guia de Testes: CI/CD e Releases

Procedimento para validar a implementação de CI/CD e releases.

## Teste 1: Validar CI (Build Automático)

### Setup
```bash
git push origin main
```

### Validação
1. Ir para **GitHub → Actions** (ou https://github.com/dduartee/esp32-event-driven-iot/actions)
2. Procurar pelo workflow **"CI - Build & Validate"** disparado pelo commit de CI
3. Verificar que o workflow **passou** ✓ (status verde)

### O que esperar
- [ ] Workflow roda com nome "CI - Build & Validate"
- [ ] Build step mostra "idf.py build" compilando com sucesso
- [ ] Artifacts são gerados e uploaddos:
  - [ ] firmware.bin
  - [ ] bootloader.bin
  - [ ] partition-table.bin
  - [ ] flasher_args.json
- [ ] Artifacts podem ser baixados na página do workflow → "Artifacts"

---

## Teste 2: Validar Script de Versionamento

Testar se o script Python funciona corretamente:

```bash
# Testar geração de version.h
python3 scripts/generate_version.py "1.2.3"

# Verificar conteúdo
cat main/version.h
```

### O que esperar
```c
// Auto-generated version header by generate_version.py
// DO NOT EDIT MANUALLY

#ifndef APP_VERSION_H
#define APP_VERSION_H

#define APP_VERSION_MAJOR 1
#define APP_VERSION_MINOR 2
#define APP_VERSION_PATCH 3
#define APP_VERSION_STRING "1.2.3"

#endif // APP_VERSION_H
```

### Testes adicionais
```bash
# Teste com versão com prefixo "v"
python3 scripts/generate_version.py "v2.0.0"
# Deve funcionar (v é removido automaticamente)

# Teste com versão inválida
python3 scripts/generate_version.py "invalid"
# Deve falhar com mensagem de erro clara
```

---

## Teste 3: Validar Release Automático

### Setup
```bash
# 1. Fazer push dos commits (se ainda não feito)
git push origin main

# 2. Criar tag de release
git tag v0.1.0

# 3. Push do tag para disparar workflow de release
git push origin v0.1.0
```

### Validação
1. Ir para **GitHub → Actions**
2. Procurar pelo workflow **"Release - Build & Publish"** disparado pelo tag v0.1.0
3. Verificar que o workflow **passou** ✓ (status verde)

### O que esperar
- [ ] Workflow roda com nome "Release - Build & Publish"
- [ ] Step "Extract version from tag" mostra: `Version extracted: 0.1.0 from tag v0.1.0`
- [ ] Step "Generate version.h" mostra o header gerado com versão 0.1.0
- [ ] Build step compila com sucesso
- [ ] Binários são renomeados com versão:
  - [ ] firmware-v0.1.0.bin
  - [ ] bootloader-v0.1.0.bin
  - [ ] partition-table-v0.1.0.bin
  - [ ] flasher_args-v0.1.0.json

---

## Teste 4: Validar GitHub Release

Após o workflow de release terminar com sucesso:

1. Ir para **GitHub → Releases** (ou https://github.com/dduartee/esp32-event-driven-iot/releases)
2. Procurar pela release **v0.1.0**

### O que esperar
- [ ] Release v0.1.0 está listada
- [ ] Release contém 4 assets:
  - [ ] firmware-v0.1.0.bin
  - [ ] bootloader-v0.1.0.bin
  - [ ] partition-table-v0.1.0.bin
  - [ ] flasher_args-v0.1.0.json
- [ ] Body da release contém instruções de flash
- [ ] Assets podem ser baixados (não são quebrados)
- [ ] URL da release é estável: `https://github.com/.../releases/download/v0.1.0/firmware-v0.1.0.bin`

---

## Teste 5: Validar Firmware Versionado

### Local
Testar se versão é embedada e compilada corretamente:

```bash
# Gerar version.h
python3 scripts/generate_version.py "0.1.0"

# Build local
idf.py build

# Verificar que version.h foi incluído
idf.py build 2>&1 | grep -i version
```

### Hardware (ESP32-C3)
Depois de gravar firmware com release v0.1.0:

```bash
idf.py monitor
```

### O que esperar no serial monitor
```
I (580) app_main: Firmware version: 0.1.0
I (590) app_main: Hello world!
```

Se não houver release (development build):
```
I (580) app_main: Firmware version: development (not from release)
```

---

## Teste 6: Validar Flash com Release Binary

### Download
```bash
cd /tmp
wget https://github.com/dduartee/esp32-event-driven-iot/releases/download/v0.1.0/firmware-v0.1.0.bin
wget https://github.com/dduartee/esp32-event-driven-iot/releases/download/v0.1.0/bootloader-v0.1.0.bin
wget https://github.com/dduartee/esp32-event-driven-iot/releases/download/v0.1.0/partition-table-v0.1.0.bin
```

### Flash com esptool.py
```bash
esptool.py --chip esp32c3 write_flash \
  0x0 /tmp/bootloader-v0.1.0.bin \
  0x8000 /tmp/partition-table-v0.1.0.bin \
  0x10000 /tmp/firmware-v0.1.0.bin
```

### Validação
- [ ] Flash não falha
- [ ] Serial monitor mostra "Firmware version: 0.1.0"
- [ ] WiFi e MQTT funcionam normalmente

---

## Checklist Final

### ✓ Implementação Completa
- [ ] CI workflow compila em cada push
- [ ] Release workflow dispara em tags v*.*.*
- [ ] Script de versionamento gera version.h corretamente
- [ ] Firmware logga versão no serial monitor
- [ ] GitHub Releases são criadas com binários
- [ ] README documenta todo o processo

### ✓ Preparação para OTA
- [ ] Versão é acessível via APP_VERSION_STRING
- [ ] Binários estão em URLs estáveis (releases.download)
- [ ] Semver facilita comparação de versões
- [ ] Base pronta para esp_https_ota futuro

### ✓ Bugs Encontrados (se algum)
- [ ] Nenhum teste falhou
- [ ] Todos os workflows passam
- [ ] Versão é embedada corretamente

---

## Troubleshooting

### CI workflow falha
1. Verificar logs em GitHub Actions
2. Checar se sdkconfig está válido: `idf.py reconfigure`
3. Verificar permissões de Docker

### Release workflow falha
1. Verificar se tag segue formato v*.*.*
2. Checar se script gera version.h válido
3. Verificar permissões para criar releases (token)

### version.h não é gerado
1. Checar se script está em `scripts/generate_version.py`
2. Executar manualmente: `python3 scripts/generate_version.py "1.0.0"`
3. Verificar se `.gitignore` ignora `main/version.h`

---

## Próximos Passos

Após validar CI/CD:

1. **Criar primeira release**: `git tag v0.1.0 && git push origin v0.1.0`
2. **Testar OTA futuro**: Implementar `esp_https_ota` em main.c
3. **Auto-checks**: Adicionar testes (pytest) no CI
4. **Notificações**: Integrar com Slack/Discord para releases


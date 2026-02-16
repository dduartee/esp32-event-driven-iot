# Quick Start: CI/CD do Zero ao Deploy

Guia rápido para começar a usar CI/CD e criar sua primeira release.

## 🚀 Passo 1: Enviar Code para GitHub (Ativar CI)

Você já tem os commits locais. Agora vamos disparar o CI:

```bash
# Ver commits não enviados
git log --oneline -5

# Enviar commits (dispara CI)
git push origin main

# Verificar CI em andamento
# → GitHub → Actions → "CI - Build & Validate" (deve estar rodando)
```

**O que esperar:**
- ✓ Workflow "CI - Build & Validate" começa a rodar
- ✓ Build compila com sucesso
- ✓ Artifacts são gerados (firmware.bin, bootloader.bin, etc)
- ✓ Status aparece no seu repo com ✅ (verde)

---

## 📦 Passo 2: Criar Primeira Release (Versionamento)

Depois que o CI passar, crie sua primeira release:

```bash
# 1. Criar tag com versão semântica
git tag v0.1.0

# 2. Enviar tag para GitHub (dispara Release workflow)
git push origin v0.1.0

# 3. Ver que é exatamente isso que foi tagado
git log --oneline | head -1
# Resultado deve mostrar: "docs: adiciona sumário executivo..."
```

**O que esperar:**
- ✓ Workflow "Release - Build & Publish" começa a rodar
- ✓ Gera version.h com versão 0.1.0
- ✓ Build compila com versão embedada
- ✓ GitHub Release v0.1.0 é criada
- ✓ Binários aparecem em: https://github.com/seu-user/esp32-event-driven-iot/releases/v0.1.0

---

## 🎯 Passo 3: Validar Release

Verificar que tudo funcionou:

```bash
# Abrir releases no navegador
# → GitHub → Releases → v0.1.0

# Ou via CLI
gh release view v0.1.0

# Listar assets
gh release view v0.1.0 --json assets
```

**Você deve ver:**
- ✓ Release "v0.1.0" listada
- ✓ 4 assets disponíveis:
  - firmware-v0.1.0.bin
  - bootloader-v0.1.0.bin
  - partition-table-v0.1.0.bin
  - flasher_args-v0.1.0.json

---

## 💾 Passo 4: Flash com Binários da Release

Baixar e gravar firmware versionado:

```bash
# Opção A: Download manual
cd /tmp
wget https://github.com/seu-user/esp32-event-driven-iot/releases/download/v0.1.0/firmware-v0.1.0.bin
wget https://github.com/seu-user/esp32-event-driven-iot/releases/download/v0.1.0/bootloader-v0.1.0.bin
wget https://github.com/seu-user/esp32-event-driven-iot/releases/download/v0.1.0/partition-table-v0.1.0.bin

# Opção B: Via CLI GitHub
gh release download v0.1.0

# Flash com esptool.py
esptool.py --chip esp32c3 write_flash \
  0x0 bootloader-v0.1.0.bin \
  0x8000 partition-table-v0.1.0.bin \
  0x10000 firmware-v0.1.0.bin
```

---

## 📺 Passo 5: Verificar Versão no Serial Monitor

Depois de gravar:

```bash
# Abrir monitor serial
idf.py monitor

# Você deve ver:
# I (580) app_main: Firmware version: 0.1.0
# I (590) app_main: Hello world!
# I (600) wifi_init: WiFi initialization...
```

Se vir "Firmware version: 0.1.0", significa:
- ✅ Version.h foi gerado corretamente
- ✅ Macros foram compiladas no firmware
- ✅ Versão está embedada e acessível no código

---

## 🔄 Passo 6: Próxima Release (Ciclo Repetido)

Para criar releases futuras:

```bash
# 1. Fazer suas mudanças
# 2. Commit (CI automático valida)
git add arquivo_modificado.c
git commit -m "feat: adiciona nova funcionalidade"
git push origin main

# 3. Quando pronto para release, criar novo tag
git tag v0.2.0
git push origin v0.2.0

# 4. GitHub Actions automaticamente:
#    ├─ Gera version.h com 0.2.0
#    ├─ Compila
#    ├─ Publica Release v0.2.0
#    └─ Assets estão prontos para download
```

---

## 🎓 Entender o Versionamento

### Semantic Versioning (Semver)

Formato: `MAJOR.MINOR.PATCH`

```
v1.2.3
  │ │ └─ PATCH: bug fixes (1.2.3 → 1.2.4)
  │ └─── MINOR: features compatíveis (1.2.3 → 1.3.0)
  └───── MAJOR: breaking changes (1.2.3 → 2.0.0)
```

**Exemplos de tags:**
- `v0.1.0` — Primeira release (projeto em beta)
- `v0.2.0` — Nova feature
- `v0.2.1` — Bug fix
- `v1.0.0` — Primeira release estável
- `v1.1.0` — Nova feature estável
- `v2.0.0` — Breaking change (ex: API diferente, novo protocolo, etc)

---

## 🐛 Troubleshooting

### CI não roda depois de push
**Causa:** GitHub Actions pode estar desativado
**Solução:**
```bash
# Verificar settings do repo
# → Settings → Actions → "Allow all actions and reusable workflows"
```

### Release workflow falha
**Causa:** Tag não segue formato v*.*.*
**Solução:**
```bash
# Deletar tag inválida
git tag -d v1.0  # Local
git push origin :refs/tags/v1.0  # Remote

# Criar tag correta
git tag v1.0.0
git push origin v1.0.0
```

### version.h não é gerado no build local
**Causa:** Você está buildando sem versão (esperado)
**Solução (opcional):**
```bash
# Se quiser testar version.h localmente:
python3 scripts/generate_version.py "1.2.3"
idf.py build
# Agora version.h existe e é compilado
```

---

## 📚 Documentação Completa

Para mais detalhes, ver:

- **README.md** — Seção "CI/CD e Releases"
- **TEST_GUIDE.md** — Testes detalhados de cada componente
- **IMPLEMENTATION_SUMMARY.md** — Arquitetura completa

---

## ✅ Checklist Final

Depois de seguir estes passos:

- [ ] `git push origin main` disparou CI (Actions → ✅)
- [ ] `git tag v0.1.0 && git push origin v0.1.0` criou release
- [ ] Release v0.1.0 aparece em GitHub → Releases
- [ ] Assets estão nomeados com versão (firmware-v0.1.0.bin, etc)
- [ ] Serial monitor mostra "Firmware version: 0.1.0"
- [ ] Você entende quando criar MAJOR/MINOR/PATCH tags

**Se tudo passou: parabéns! 🎉 CI/CD está pronto para produção.**

---

## 🚀 O Próximo Passo Recomendado

Agora que CI/CD está funcional, você pode:

1. **Implementar OTA Updates** — Usar esp_https_ota com URLs de release
2. **Adicionar testes automáticos** — pytest no CI
3. **Auto-changelog** — Gerar release notes automaticamente
4. **Notificações** — Slack/Discord quando release é criada

---

*Para começar: `git push origin main`*

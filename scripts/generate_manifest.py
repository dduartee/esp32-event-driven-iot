#!/usr/bin/env python3
"""
Gera manifest.json com metadata de OTA para GitHub Release.
Uso: python scripts/generate_manifest.py <firmware.bin> <version> <base_url>

Exemplo:
    python scripts/generate_manifest.py \\
        build/esp32-event-driven-iot.bin \\
        0.2.0 \\
        https://github.com/gabrielkduarte/esp32-event-driven-iot/releases/download/v0.2.0
"""

import sys
import hashlib
import json
from pathlib import Path


def calculate_sha256(file_path):
    """Calcula SHA256 do firmware"""
    sha256 = hashlib.sha256()
    with open(file_path, 'rb') as f:
        while chunk := f.read(8192):
            sha256.update(chunk)
    return sha256.hexdigest()


def generate_manifest(firmware_path, version, base_url):
    """Gera manifest.json com metadata de OTA"""
    firmware_path = Path(firmware_path)

    if not firmware_path.exists():
        raise FileNotFoundError(f"Firmware file not found: {firmware_path}")

    manifest = {
        "version": version,
        "firmware_url": f"{base_url}/esp32-event-driven-iot-v{version}.bin",
        "sha256": calculate_sha256(firmware_path),
        "size": firmware_path.stat().st_size,
        "release_notes": f"Release {version}"
    }

    return manifest


def main():
    if len(sys.argv) < 4:
        print("Uso: python scripts/generate_manifest.py <firmware.bin> <version> <base_url>")
        print()
        print("Exemplo:")
        print("  python scripts/generate_manifest.py \\")
        print("    build/esp32-event-driven-iot.bin \\")
        print("    0.2.0 \\")
        print("    https://github.com/.../releases/download/v0.2.0")
        sys.exit(1)

    firmware = sys.argv[1]
    version = sys.argv[2]
    base_url = sys.argv[3]

    try:
        manifest = generate_manifest(firmware, version, base_url)

        # Criar diretório build/ se não existir
        build_dir = Path("build")
        build_dir.mkdir(exist_ok=True)

        # Escrever manifest.json
        output = build_dir / "manifest.json"
        output.write_text(json.dumps(manifest, indent=2))

        print(f"✓ Manifest gerado: {output}")
        print()
        print("Conteúdo:")
        print(json.dumps(manifest, indent=2))
        return 0

    except Exception as e:
        print(f"✗ Erro ao gerar manifest: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())

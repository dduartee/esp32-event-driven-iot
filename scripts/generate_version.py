#!/usr/bin/env python3
"""
Gera main/version.h com macros de versão a partir de semver string.
Uso: python scripts/generate_version.py "1.0.0"
Saída: main/version.h com APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH, APP_VERSION_STRING
"""

import sys
import re
from pathlib import Path


def validate_semver(version_str):
    """Valida se a string segue o formato semver (MAJOR.MINOR.PATCH)"""
    pattern = r"^(\d+)\.(\d+)\.(\d+)$"
    match = re.match(pattern, version_str)
    if not match:
        raise ValueError(
            f"Versão inválida: {version_str}. Esperado: MAJOR.MINOR.PATCH (ex: 1.0.0)"
        )
    return int(match.group(1)), int(match.group(2)), int(match.group(3))


def generate_version_h(version_str, output_path):
    """Gera header C com macros de versão"""
    major, minor, patch = validate_semver(version_str)

    header_content = f"""// Auto-generated version header by generate_version.py
// DO NOT EDIT MANUALLY

#ifndef APP_VERSION_H
#define APP_VERSION_H

#define APP_VERSION_MAJOR {major}
#define APP_VERSION_MINOR {minor}
#define APP_VERSION_PATCH {patch}
#define APP_VERSION_STRING "{version_str}"

#endif // APP_VERSION_H
"""

    output_path.write_text(header_content)
    print(f"✓ Versão {version_str} gerada em {output_path}")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Uso: python scripts/generate_version.py <VERSION>")
        print("Exemplo: python scripts/generate_version.py 1.0.0")
        sys.exit(1)

    version = sys.argv[1].lstrip("v")  # Remove "v" prefixo se houver (v1.0.0 -> 1.0.0)
    output = Path(__file__).parent.parent / "main" / "version.h"

    try:
        generate_version_h(version, output)
    except ValueError as e:
        print(f"✗ Erro: {e}", file=sys.stderr)
        sys.exit(1)

Import("env")

from pathlib import Path

project_dir = Path(env["PROJECT_DIR"])
version = (project_dir / "version.txt").read_text().strip()
env.Append(CPPDEFINES=[("POGLIGHT_FW_VERSION", env.StringifyMacro(version))])

# Genere l'asset C dans le repertoire de build : l'icone reste un PNG normal
# dans le depot, tout en etant servie directement depuis la flash de l'ESP32.
icon = (project_dir / "assets" / "brand" / "icon.png").read_bytes()
generated_dir = Path(env.subst("$BUILD_DIR")) / env["PIOENV"] / "generated"
generated_dir.mkdir(parents=True, exist_ok=True)
header = generated_dir / "poglight_icon.h"
rows = [
    ",".join(f"0x{byte:02x}" for byte in icon[offset:offset + 16])
    for offset in range(0, len(icon), 16)
]
header.write_text(
    "#pragma once\n#include <Arduino.h>\n"
    "const uint8_t POGLIGHT_ICON[] PROGMEM = {\n  "
    + ",\n  ".join(rows)
    + "\n};\n"
    + f"const size_t POGLIGHT_ICON_LEN = {len(icon)};\n"
)
env.Append(CPPPATH=[str(generated_dir)])

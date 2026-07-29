Import("env")

from pathlib import Path

version = (Path(env["PROJECT_DIR"]) / "version.txt").read_text().strip()
env.Append(CPPDEFINES=[("POGLIGHT_FW_VERSION", env.StringifyMacro(version))])

Import("env")

from pathlib import Path
import re


def read_values(path):
    values = {}
    if not path.exists():
        return values
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip()
        if not re.fullmatch(r"[A-Z][A-Z0-9_]*", key):
            raise ValueError("Invalid .env key: " + key)
        if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
            value = value[1:-1]
        values[key] = value
    return values


def c_string(value):
    escaped = value.replace("\\", "\\\\").replace('"', '\\"')
    escaped = escaped.replace("\r", "\\r").replace("\n", "\\n")
    return '"' + escaped + '"'


project_dir = Path(env.subst("$PROJECT_DIR"))
generated_dir = project_dir / ".pio" / "generated"
generated_dir.mkdir(parents=True, exist_ok=True)
header = generated_dir / "EnvConfig.h"
values = read_values(project_dir / ".env")
lines = ["#pragma once", ""]
for name, value in sorted(values.items()):
    lines.append("#define {} {}".format(name, c_string(value)))
header.write_text("\n".join(lines) + "\n", encoding="utf-8")
framework_dir = Path(env.PioPlatform().get_package_dir("framework-arduinoespressif32"))
fs_include_dir = framework_dir / "libraries" / "FS" / "src"
env.Append(CPPPATH=[str(generated_dir), str(fs_include_dir)])
if values.get("OTA_PASSWORD"):
    env.Append(CPPDEFINES=[("OTA_PASS", c_string(values["OTA_PASSWORD"]))])

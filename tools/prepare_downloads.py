#!/usr/bin/env python3
"""Regenera les descàrregues de la guia amb la biblioteca estàndard de Python."""
from pathlib import Path
import hashlib
import json
import shutil
import zipfile

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'docs' / 'downloads'
OUT.mkdir(parents=True, exist_ok=True)
FILES = {
    'touchdesigner/Actualitza_sensor_real.py': 'Actualitza_sensor_real.py',
    'firmware/ESP32_OSC_Demo/ESP32_OSC_Demo.ino': 'ESP32_OSC_Demo.ino',
    'touchdesigner/Prova_ESP32_TouchDesigner.toe': 'Prova_ESP32_TouchDesigner.toe',
    'touchdesigner/Prova_ESP32_TouchDesigner.tox': 'Prova_ESP32_TouchDesigner.tox',
    'hardware/esquema.svg': 'Esquema_ESP32_HCSR04.svg',
    'hardware/esquema.png': 'Esquema_ESP32_HCSR04.png',
    'hardware/GUIA_SOLDADURA.md': 'Guia_de_connexio_i_soldadura.md',
}
for source, destination in FILES.items():
    shutil.copyfile(ROOT / source, OUT / destination)
with zipfile.ZipFile(OUT / 'ESP32_OSC_Demo.zip', 'w', zipfile.ZIP_DEFLATED) as archive:
    info = zipfile.ZipInfo('ESP32_OSC_Demo/ESP32_OSC_Demo.ino', (2026, 11, 7, 0, 0, 0))
    info.compress_type = zipfile.ZIP_DEFLATED
    info.external_attr = 0o644 << 16
    archive.writestr(info, (ROOT / 'firmware/ESP32_OSC_Demo/ESP32_OSC_Demo.ino').read_bytes())
manifest = {
    p.name: hashlib.sha256(p.read_bytes()).hexdigest()
    for p in sorted(OUT.iterdir()) if p.is_file() and p.name != 'checksums.json'
}
(OUT / 'checksums.json').write_text(json.dumps(manifest, indent=2) + '\n')
print(f'{len(manifest)} descàrregues preparades.')

#!/usr/bin/env python3
"""Comprova els enllaços locals, les còpies de firmware i el ZIP descarregable."""
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit
import hashlib
import json
import re
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SITE = ROOT / 'docs'

class Links(HTMLParser):
    def __init__(self):
        super().__init__()
        self.refs = []
        self.ids = []
    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if 'id' in attrs:
            assert attrs['id'] not in self.ids, f'ID duplicat: {attrs["id"]}'
            self.ids.append(attrs['id'])
        for key in ['href', 'src', 'data-zoom']:
            if attrs.get(key): self.refs.append(attrs[key])

parser = Links()
parser.feed((SITE / 'index.html').read_text())
quick = Links()
quick.feed((SITE / 'comenca.html').read_text())
assert 'index.html#wifi' in quick.refs
assert 'index.html#touchdesigner' in quick.refs
assert 'downloads/Prova_ESP32_TouchDesigner.toe' in quick.refs
assert 'assets/qr-guia-imprimible.pdf' in quick.refs
for ref in parser.refs + quick.refs:
    url = urlsplit(ref)
    if url.scheme or url.netloc: continue
    if not url.path:
        assert not url.fragment or url.fragment in parser.ids, f'Àncora inexistent: {ref}'
    else:
        assert (SITE / unquote(url.path)).is_file(), f'Fitxer inexistent: {ref}'

source = (ROOT / 'firmware/ESP32_OSC_Demo/ESP32_OSC_Demo.ino').read_bytes()
assert source == (SITE / 'downloads/ESP32_OSC_Demo.ino').read_bytes()
assert b'void setup()' in source and b'void loop()' in source
assert b'String oscHost = ""' in source, 'No deixis una destinació OSC d’una xarxa privada com a valor inicial.'
with zipfile.ZipFile(SITE / 'downloads/ESP32_OSC_Demo.zip') as z:
    assert z.namelist() == ['ESP32_OSC_Demo/ESP32_OSC_Demo.ino']
    assert z.read(z.namelist()[0]) == source
for name, expected in json.loads((SITE / 'downloads/checksums.json').read_text()).items():
    assert hashlib.sha256((SITE / 'downloads' / name).read_bytes()).hexdigest() == expected
js = (SITE / 'app.js').read_text()
for target in re.findall(r"\$\('#([^']+)'\)", js):
    assert target in parser.ids, f'Element JS inexistent: {target}'
for ext in ['toe', 'tox']:
    name = f'Prova_ESP32_TouchDesigner.{ext}'
    assert (ROOT / 'touchdesigner' / name).read_bytes() == (SITE / 'downloads' / name).read_bytes()
print(f'OK: {len(parser.refs) + len(quick.refs)} referències HTML; guia ràpida, QR, firmware, TouchDesigner, ZIP i sumes de verificació coherents.')

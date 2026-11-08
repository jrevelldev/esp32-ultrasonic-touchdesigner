"""Executa a Textport amb exec(open('/ruta/Actualitza_sensor_real.py').read()).
Actualitza etiquetes, validesa i afegeix canals raw/filtrats; conserva els nodes existents.
"""
from pathlib import Path
import json

c=op('/project1/esp32_demo')
if c is None:
    raise RuntimeError('Obre primer el projecte ESP32 TouchDesigner.')
c.op('helpers').text='''import time

def value(suffix, default=0.0):
    try:
        for channel in me.parent().op('osc_in').chans():
            if channel.name.endswith(suffix):
                return float(channel[0])
    except Exception:
        pass
    return default

def online():
    return time.monotonic() - me.parent().fetch('last_seen', -1e12) < 1.5

def valid():
    return online() and value('valid', 1) > 0.5 and value('distancia', -1) >= 0

def source_label():
    if not online(): return 'ESPERANT DADES'
    return 'DISTÀNCIA SIMULADA' if value('simulat', 1) > 0.5 else 'DISTÀNCIA REAL'

def distance_text():
    return ('%.1f cm' % value('distancia')) if valid() else '-- cm'

def status_text():
    if not online(): return 'ESPERANT L’ESP32  /  OSC'
    if value('simulat', 1) > 0.5: return 'REBENT OSC  /  SENSOR SIMULAT'
    return 'REBENT OSC  /  SENSOR REAL' if valid() else 'SENSOR REAL  /  SENSE ECO VÀLID'
'''
c.op('label').par.text.expr="op('helpers').module.source_label()"
c.op('footer').par.text='20 cm: molt a prop  ·  200 cm: lluny\n\nHC-SR04 · TRIG GPIO4 · ECHO GPIO3\nOSC /sensor/distancia · /sensor/proximitat'
# Canals separats per comparar el senyal i decidir on filtrar.
rx=c.op('osc_in')
for name,pattern,y in [('raw_cm','*raw',-550),('filtrada_esp_cm','*filtrada',-650),('validesa','*valid',-750),('interval_ms','*interval_ms',-850),('echo_us','*echo_us',-950)]:
    node=c.op(name) or c.create(selectCHOP,name)
    node.inputConnectors[0].connect(rx)
    node.par.channames=pattern
    node.par.renamefrom='*'
    node.par.renameto=name
    node.nodeX,node.nodeY=450,y
    node.viewer=True
lag=c.op('suavitzat_td') or c.create(lagCHOP,'suavitzat_td')
lag.inputConnectors[0].connect(c.op('raw_cm'))
lag.par.lagunit = 'seconds'
lag.par.lag1=0.15
lag.par.lag2=0.15
lag.nodeX,lag.nodeY=700,-550
lag.viewer=True
c.op('LLEGEIX_ME').text = '''ESP32-C3 · HC-SR04 · OSC

raw_cm: distància sense filtre en cm.
filtrada_esp_cm: sortida EMA de l’ESP (igual a raw si desactivat).
suavitzat_td: Lag CHOP de prova de 0,15 segons sobre raw_cm.
validesa: 1 mesura vàlida; 0 sense eco. No utilitzis −1 com a distància.
interval_ms: interval real entre mesures. echo_us: pols ECHO en microsegons.
distancia_cm i el panell utilitzen la sortida seleccionada a la web de l’ESP.

A la web: Lectura i suavitzat → Raw per filtrar només a TD.
Les dades venen de l’ESP; TD no genera cap distància.
'''
errors={n.name:n.errors() for n in c.children if n.errors()}
if errors: raise RuntimeError(str(errors))
out=Path(project.folder)
c.save(str(out/'Prova_ESP32_TouchDesigner.tox'))
project.save(str(out/'Prova_ESP32_TouchDesigner.toe'))
print('Panell actualitzat: raw, EMA ESP, Lag TD 0,15 s; errors:', errors)
print('Canals raw:', [ch.name for ch in c.op('raw_cm').chans()], 'Lag unit:', lag.par.lagunit.eval())

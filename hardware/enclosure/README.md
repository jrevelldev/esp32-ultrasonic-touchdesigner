# Caixa imprimible per al sensor

Aquesta carpeta conté els fitxers aportats per a una caixa de l’ESP32-C3 Super Mini i el HC-SR04. Els STL de la caixa són `sensor_box_body.stl` i `sensor_box_lid.stl`.

- `sensor_box_body.stl`, `sensor_box_lid.stl`: peces per preparar al laminador i imprimir.
- `sensor_box.blend`: distribució de les peces a Blender.
- `sensor_box_assembled.blend`: vista del conjunt muntat.
- `electronics_model.stl`: maqueta de l’electrònica per comprovar l’encaix; no és una peça necessària per a la caixa.
- `sensor_box.py`: generador paramètric de Blender. En executar-lo, **esborra l’escena oberta**; fes-lo servir en un fitxer nou o després de desar el teu treball.
- `muntatge_explotat.png`, `vista_frontal.png`, `seccio_verificacio.png`: imatges de referència.

Les mides del generador corresponen al muntatge per al qual es va preparar. Abans d’imprimir una altra unitat, comprova les dimensions del teu sensor, de la placa perforada, de l’ESP i del connector USB. Fes una prova d’encaix abans de tancar la caixa definitivament.

[Descarrega tots els fitxers en un ZIP](../../docs/downloads/sensor-box-3D-files.zip).

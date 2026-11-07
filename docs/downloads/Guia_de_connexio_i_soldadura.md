# ESP32-C3 Super Mini i HC-SR04 de 5 V

Aquesta guia correspon al sensor HC-SR04 estàndard de **5 V amb quatre pins: VCC, TRIG, ECHO i GND**, com el de la captura que vas mostrar. Abans de soldar, comprova aquestes etiquetes al sensor real. L'esquema és funcional: la posició dibuixada dels terminals no representa la posició dels forats de la teva placa.

## Material

- ESP32-C3 Super Mini i HC-SR04 de 5 V.
- R1: una resistència de **2,2 kΩ**.
- R2: una resistència de **3,3 kΩ**.
- Resistències d'1/4 W; una tolerància del 5 % és suficient per a aquest divisor.
- Quatre cables flexibles per al sensor, inicialment curts, d'uns 10–20 cm.
- Una petita placa perforada per fixar les dues resistències i les unions.
- Estany, soldador de punta fina, flux adequat per a electrònica i tub termoretràctil.
- Multímetre, si en tens, per comprovar valors i curtcircuits.
- Tira de pins o connectors si vols que les plaques es puguin desconnectar.

Opcional: un condensador ceràmic de **100 nF** entre VCC i GND, prop del sensor. No té polaritat. No és necessari afegir-lo abans de la primera prova amb cables curts.

## Connexions

| Terminal del sensor | Connexió a l'ESP |
| --- | --- |
| VCC | Pin **5V**, amb l'ESP alimentat per USB-C |
| GND | Pin **G / GND** |
| TRIG | Pin **4 / GPIO4**, directament |
| ECHO | A **R1 de 2,2 kΩ**; l'altre extrem de R1 és el punt central del divisor |
| Punt central del divisor | Pin **3 / GPIO3**, i un extrem de **R2 de 3,3 kΩ** |
| Altre extrem de R2 | **GND** comú |

El pin VCC d'aquest sensor va a **5 V**, no al pin 3V3 de la placa. Els GPIO de l'ESP treballen amb 3,3 V. TRIG pot rebre el senyal de l'ESP, però el retorn ECHO de 5 V necessita el divisor. [Guia del HC-SR04 d'Adafruit](https://learn.adafruit.com/ultrasonic-sonar-distance-sensors?view=all).

El divisor és:

```text
ECHO del sensor ── R1 2,2 kΩ ──●──── GPIO3 de l'ESP
                              │
                           R2 3,3 kΩ
                              │
                             GND
```

Amb ECHO a 5 V, el punt central queda aproximadament a **3,0 V**:

`5 × 3,3 / (2,2 + 3,3) = 3,0 V`

Les resistències no tenen polaritat. Sí que importa quin valor fa de R1 i quin fa de R2.

## Ordre de muntatge i soldadura

1. **Desconnecta l'USB i qualsevol altra alimentació.** Treballa en un espai ventilat. Si tens protoboard i connectors, fes primer el muntatge provisional.
2. Identifica les etiquetes 5V, G/GND, 4 i 3 de l'ESP i les quatre etiquetes del sensor. Si els connectors apunten cap a l'altra cara, l'ordre visual es pot veure invertit: segueix sempre la serigrafia.
3. Comprova els valors de les resistències abans de muntar-les. Col·loca-les a la placa perforada, preferentment prop de l'ESP.
4. **Crea el punt central:** uneix un extrem de R1 de 2,2 kΩ, un extrem de R2 de 3,3 kΩ i el cable que anirà a GPIO3. Aquesta és una unió de tres terminals.
5. Connecta l'extrem lliure de R1 al cable ECHO del sensor.
6. Connecta l'extrem lliure de R2 al GND comú. En aquest GND també han d'estar units el GND del sensor i el GND de l'ESP.
7. Connecta TRIG a GPIO4 i VCC a 5V. Pots utilitzar vermell per a 5V, negre per a GND, blau per a TRIG i verd per a ECHO.
8. Per soldar cada unió, estanya lleugerament el cable, escalfa alhora el terminal i el coure, aporta una mica d'estany i retira el soldador quan hagi fluït. Si no flueix al cap de pocs segons, deixa refredar i revisa la punta i el flux.
9. Inspecciona que no hi hagi ponts d'estany entre pins ni fils solts. Aïlla les unions i subjecta el cable al suport o a la caixa perquè una estrebada no arrenqui els pads.

Si la placa perforada té pistes de coure contínues, comprova que no uneixin accidentalment els dos extrems d'una resistència. La connexió de la placa ha de reproduir l'esquema.

## Abans d'alimentar

- Comprova que VCC i GND no estiguin intercanviats i que no hi hagi un curtcircuit entre 5V i GND.
- Comprova que ECHO arribi a GPIO3 passant pel punt central del divisor.
- Comprova que R2 acaba a GND i que tots els GND estan units.
- Alimenta inicialment tot el conjunt des de l'USB-C de l'ESP. No hi afegeixis una segona font de 5 V al mateix temps.
- Deixa l'antena de l'ESP separada de metall i dels cables tant com sigui possible, i les dues càpsules del sensor lliures davant seu.

Per comprovar el divisor amb un multímetre abans de connectar-lo al GPIO: amb el sensor desconnectat i GPIO3 encara desconnectat, es pot provar el divisor amb 5 V a l'extrem lliure de R1 i GND a l'extrem lliure de R2. El punt central hauria de marcar prop de 3 V respecte de GND. Desconnecta l'alimentació i retira aquesta connexió temporal a 5 V abans de connectar ECHO i GPIO3 segons l'esquema.

## Programa de la placa

El programa actual continua generant **distàncies simulades**. Connectar el sensor encara no farà que la web o TouchDesigner mostrin una lectura real. Quan el cablejat estigui revisat, cal afegir la lectura de TRIG a GPIO4 i ECHO a GPIO3 i canviar l'indicador de simulació.

La configuració Wi-Fi, el nom individual, el LED d'estat i els missatges OSC es poden conservar. [Referència de pins de la Super Mini](https://nuttx.apache.org/docs/latest/platforms/risc-v/esp32c3/boards/esp32c3-supermini/index.html).

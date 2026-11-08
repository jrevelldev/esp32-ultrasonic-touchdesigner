# Patch ESP32 → TouchDesigner

## Obre el patch

- **Prova_ESP32_TouchDesigner.toe**: projecte complet. Desa primer qualsevol projecte obert amb canvis.
- **Prova_ESP32_TouchDesigner.tox**: component per arrossegar a la xarxa d’un projecte existent.
- Versió comprovada: TouchDesigner 2025.32460. No cal instal·lar biblioteques Python ni OSC.

## Connecta l’ESP

1. Connecta l’ordinador i l’ESP al mateix Wi-Fi.
2. A la web de l’ESP, a **TouchDesigner · OSC**, desa la **IP de l’ordinador** i el port **9000**.
3. Al patch, entra a `esp32_demo` i comprova `osc_in`: Active On, Network Port 9000.
4. Evita tenir dues còpies escoltant el mateix port. Per a diversos sensors, usa ports diferents.

El panell identifica Sensor real / Simulat. «Sense eco vàlid» no és una distància: el canal `validesa` val 0.

## Ajusta la bola visualment

A `/project1`, selecciona el component **`esp32_demo`** i obre la pestanya de paràmetres **Ball Controls**. El visor exterior del component mostra el panell complet amb la lectura i el cercle.

- **Near (cm)** i **Far (cm)**: límits de distància. Per exemple, 5 i 300 cm; ajusta’ls a la teva instal·lació.
- **Small size** i **Large size**: radis mínim i màxim.
- **Smoothing (s)**: temps de transició de la bola; 0,25 s per defecte. Ajusta el Lag CHOP `ball_smooth`, també quan es perd el senyal.
- **Near makes large**: activa per tenir la bola gran a prop; desactiva per tenir-la petita a prop.

La bola usa la distància `raw_cm` com a objectiu visual. Si no arriba un eco vàlid o no hi ha dades recents, l’objectiu passa a **Far (cm)** i la mida hi arriba gradualment segons **Smoothing (s)**. El text mostra `-- cm` en aquest cas. No cal editar cap fórmula ni tornar a programar l’ESP.

## Canals útils

- `raw_cm`: distància sense filtre, en cm.
- `filtrada_esp_cm`: distància filtrada a l’ESP si hi actives EMA.
- `suavitzat_td`: Lag CHOP aplicat a la distància raw, disponible per als teus efectes.
- `ball_target`: objectiu visual; si no hi ha senyal, apunta a «lluny».
- `ball_smooth`: transició gradual de la mida, ajustada amb **Smoothing (s)**.
- `distancia_cm`: sortida principal triada a la web de l’ESP.
- `proximitat`: canal de l’ESP de 0 lluny (200 cm) a 1 a prop (20 cm); **no** defineix els límits de la bola.
- `validesa`: 1 amb lectura vàlida, 0 sense eco vàlid.

[Guia ràpida de Wi-Fi i descàrrega](https://jrevelldev.github.io/esp32-ultrasonic-touchdesigner/comenca.html) · [Guia completa](https://jrevelldev.github.io/esp32-ultrasonic-touchdesigner/)

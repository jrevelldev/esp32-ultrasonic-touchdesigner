# Patch ESP32 → TouchDesigner · v1.2.2

## Obre el patch

- **Prova_ESP32_TouchDesigner.toe**: projecte complet. Desa primer qualsevol projecte obert amb canvis.
- **Prova_ESP32_TouchDesigner.tox**: component per arrossegar a la xarxa d’un projecte existent.
- Versió comprovada: TouchDesigner 2025.32460. No cal instal·lar biblioteques Python ni OSC.

## Rep les dades

1. Connecta l’ordinador i l’ESP al mateix Wi-Fi.
2. A la web de l’ESP, a **TouchDesigner · OSC**, desa la **IP de l’ordinador** i el port **9000**.
3. Al patch, entra a `esp32_demo` i comprova `osc_in`: Active On, Network Port 9000.
4. Evita tenir dues còpies escoltant el mateix port. Per a diversos sensors, configura ports diferents a cada parella ESP/receptor.
5. El panell identifica Sensor real / Simulat. «Sense eco vàlid» no és una distància: el canal `validesa` val 0.

## Bola amb moviment suau

El node **mida_suau** fa la transició de la mida amb un Lag CHOP. **Lag Up = 0,25**, **Lag Down = 0,25**, unitats **Seconds**. Puja els valors per a més suavitat i retard; baixa’ls per a més resposta. Actua sobre la mida visual; els canals de distància continuen disponibles.

- `raw_cm`: distància sense filtre, en cm.
- `filtrada_esp_cm`: distància filtrada a l’ESP, si hi actives EMA.
- `suavitzat_td`: exemple separat de suavitzat de la distància a TD (0,15 s).
- `distancia_cm`: sortida principal triada a la web de l’ESP.
- `proximitat`: 0 lluny (200 cm), 1 a prop (20 cm).

Per filtrar només a TD, deixa **Raw · sense filtre** a la web de l’ESP. El patch rep les dades del dispositiu; no genera distàncies pròpies.

[Guia de muntatge i configuració des de zero](https://jrevelldev.github.io/esp32-ultrasonic-touchdesigner/)

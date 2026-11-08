# Validació i límits

## Firmware 1.1.0 · 8 de novembre de 2026

- Compilat amb esp32 3.3.12, ESP32C3 Dev Module i USB CDC activat.
- Carregat per Arduino IDE amb esborrat complet de flash desactivat.
- Verificat per HTTP: versió 1.1.0, `simulated: false`, `valid: true` i lectura física inicial de 251,59 cm. Això confirma recepció d’ecos, no una calibració amb regla.
- Wi-Fi, IP i destinació OSC conservats després de carregar.
- Impuls TRIG de 10 µs, mesures cada 100 ms, timeout ECHO de 30 ms. S’accepta l’interval nominal 2–400 cm.
- Sense eco o fora de rang: distància JSON `null`, distància OSC −1, proximitat 0 i `/sensor/valid = 0`.
- Simulacions automàtica i manual seleccionables explícitament; arrencada sempre en sensor real.

## Compatibilitat i prova original

Es conserven les adreces OSC de distància, proximitat, simulació i seqüència. S’afegeix `/sensor/valid`. El panell TouchDesigner actualitzat evita mostrar la lectura invàlida com una distància. Les versions antigues es conserven a `archive/firmware-simulat-1.0/`.

La proximitat satura a 0 a partir de 200 cm i a 1 per sota de 20 cm, encara que la distància real pugui ser vàlida fora d’aquest interval.

Les proves de 42 i 155 cm de la primera versió eren simulades, no lectures físiques. La precisió i l’abast del sensor físic no s’han calibrat: cal contrastar-los amb distàncies conegudes.

## Firmware 1.2.0 · raw i adquisició configurable

- Compilat amb esp32 3.3.12 i CDC: 1.057.519 bytes de programa (80 %); 38.848 bytes globals (11 %).
- Carregat a la mateixa placa i versió 1.2.0 confirmada per HTTP; Wi-Fi i destinació OSC conservats.
- Formularis amb interval inferior a 70 ms, filtre desconegut i alpha zero rebutjats amb HTTP 400.
- Prova manual de 42 cm: raw, filtrada i sortida principal coincideixen amb filtre desactivat.
- EMA alpha 0,25: 34 parells de mostres consecutives comprovats en simulació automàtica; error màxim 0,0075 cm respecte de la recurrència (arrodoniment JSON).
- Interval configurat 70 ms: mediana observada 74 ms. Un enviament OSC per mostra nova; l’interval real es publica.
- Estat final de la prova: sensor real, interval 70 ms, filtre desactivat, alpha 0,25. L’absència d’eco es marca invàlida.

El límit de ritme segueix el HC-SR04, no els FPS del receptor. Els assajos del filtre no són una calibració física del sensor. Els ajustos d’adquisició es guarden a NVS; no s’ha fet una prova separada de desconnexió d’alimentació en aquesta revisió.

## Firmware 1.2.1 · pauses periòdiques

El diagnòstic USB podia esperar fins a 20 × 100 ms quan el host no consumia el text (HWCDC del core instal·lat). Es limita el timeout USB a zero i el missatge periòdic només s’escriu si hi ha espai. La web fa una sola consulta de telemetria en curs, amb timeout i missatge de reconnexió visible. Compilat: 1.058.451 bytes (80 %) i 38.848 bytes globals (11 %).

TouchDesigner: canals raw_cm, filtrada_esp_cm, suavitzat_td, validesa, interval_ms i echo_us creats, amb un canal cadascun i sense errors. Lag Up/Down 0,15 i unitat seconds confirmats; .toe i .tox desats. La configuració d’adquisició 70 ms/raw/alpha 0,25 s’ha conservat durant la càrrega de 1.2.1.

Prova de 25 s després de carregar 1.2.1, amb el monitor sèrie tancat i la web oberta: 161 respostes HTTP, cap error, latència mediana 47 ms i màxima 204 ms (abans: màxima 2.285 ms). 338 mostres en 24.876 ms de temps de mostreig; interval observat mediana 74 ms. 141 respostes amb eco vàlid. Les pauses periòdiques de dos segons no s’han reproduït en aquesta finestra de prova.

# Comprovacions de la guia · 7 de novembre de 2026

- Firmware reutilitzable compilat amb ESP32 3.3.12 i `esp32:esp32:esp32c3:CDCOnBoot=cdc`.
- Resultat de compilació: **1.049.837 bytes (80 %)** de programa i **38.808 bytes (11 %)** de variables globals. Aquesta edició només canvia la destinació OSC inicial per deixar-la buida; no s’ha tornat a carregar a la placa durant la preparació de la guia.
- JavaScript validat amb `node --check docs/app.js`.
- Enllaços i àncores locals comprovats amb `python3 tools/check_site.py`.
- Codi font, fitxer descarregable i contingut del ZIP comparats byte per byte; el ZIP inclou la carpeta que necessita Arduino.
- Projectes TouchDesigner descarregables comparats amb els fitxers validats de la prova original.
- Proves en navegador: navegar entre passos, marcar un pas, recuperar el progrés després de recarregar, copiar l’URL del gestor de plaques i copiar el codi complet.
- Notes: rebuig d’una IP invàlida, desat de les notes i recuperació després de recarregar.
- Diagrama de xarxa: canvi entre configuració i ús normal. Il·lustració: obrir i tancar l’ampliació.
- Revisió visual en escriptori i a **390 × 844 px**; els deu passos no provoquen desbordament horitzontal de la pàgina.
- Sense errors de consola observats en la revisió del navegador.
- Les dues il·lustracions aportades s’han copiat sense modificar-les. La guia indica que són conceptuals i que cal seguir les etiquetes de les plaques reals.

La lectura del sensor HC-SR04 físic continua pendent, tal com s’explica a [VALIDACIO.md](VALIDACIO.md).

## Actualització del cablejat i de les referències

- Incorporades tres captures originals aportades per l’usuari (frontal, posterior i HC-SR04), sense retocar-les.
- Il·lustració principal refeta amb la placa frontal correcta i el sensor en vertical, segons la preferència de l’usuari.
- Revisades les quatre connexions del sensor, els pins 5V/GND/GPIO4/GPIO3 de la Super Mini i el divisor R1/R2; GND no està connectat a GPIO0.
- Esquema SVG actualitzat i exportació PNG regenerada des del mateix esquema.
- Afegida l’explicació de 5 V enfront de 3,3 V amb fonts d’Adafruit i l’alternativa US-100 alimentat a 3,3 V en mode TRIG/ECHO.
- Comprovada l’ampliació de la nova il·lustració en navegador, les tres referències i l’absència de desbordament de la pàgina a 390 px.
- Firmware i fitxers TouchDesigner sense canvis en aquesta actualització.

## Correcció del moviment de la web · 8 de novembre de 2026

- El mode, la destinació OSC i el comptador passen a tres files fixes, amb xifres d’amplada constant.
- Verificat en navegador amb 9, 9.999, 10.000 i 4.294.967.295 lectures, i amb destinació buida. En tots els casos el panell fa 71 px i el formulari conserva la mateixa posició a cada amplada comprovada (1280 i 320 px).
- Compilació ESP32-C3 amb USB CDC: 1.050.589 bytes de programa, 38.808 bytes de variables globals.
- Correcció aplicada i desada al projecte Arduino `esp32C3_wifiUltrasonicSensor`; càrrega per Arduino IDE amb verificació correcta i esborrat complet de flash desactivat.
- Després de carregar, la web de la placa inclou els nous camps `telemetry-mode`, `telemetry-destination` i `telemetry-count`. La IP i la destinació OSC s’han conservat.
- La lectura continua sent simulada; aquest canvi només afecta la presentació web.

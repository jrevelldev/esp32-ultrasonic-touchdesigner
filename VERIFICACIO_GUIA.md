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

# Validació i límits

## Prova original amb la placa

Amb una ESP32-C3 Super Mini, Arduino IDE 2.3.10, paquet esp32 3.3.12 i TouchDesigner 2025.32460 es va comprovar:

- Càrrega del programa, monitor sèrie i botó BOOT.
- Connexió Wi-Fi, web local i configuració d’IP fixa.
- Nom de configuració individual derivat de la MAC completa.
- Enviament real ESP → Wi-Fi → TouchDesigner, amb distància **simulada a l’ESP**.
- Valors manuals de 42 cm (proximitat ≈0,878) i 155 cm (0,25).
- Detecció de pèrdua de seqüència al receptor i recuperació en restaurar el port correcte.
- Reobertura del projecte TouchDesigner desat.

Els fitxers de la primera prova es conserven a `archive/`. Les seves IP corresponen a aquella xarxa, no són valors per a tots els muntatges.

## Edició per repetir el projecte

La còpia reutilitzable del firmware elimina la IP privada de l’ordinador que hi havia com a destinació OSC inicial: ara queda buida fins que la configures a la web. No modifica els noms dels missatges OSC, el format de la memòria, la configuració de xarxa ni els pins.

La compilació d’aquesta edició i les comprovacions de la web es documenten a `VERIFICACIO_GUIA.md`. Una compilació correcta no equival a una nova prova amb el sensor físic.

## Pendent

El firmware actual no llegeix GPIO3/GPIO4: només genera distàncies simulades. El cablejat i la soldadura estan documentats, però no consta una prova amb el HC-SR04 connectat. Cal integrar-ne la lectura, gestionar absència d’eco i valors fora de rang, validar-los físicament i actualitzar l’indicador `/sensor/simulat` abans de considerar-lo un sensor operatiu.

Les il·lustracions generades són conceptuals. La taula de connexions i l’esquema funcional especifiquen el circuit; la serigrafia de la placa real especifica els pins físics.

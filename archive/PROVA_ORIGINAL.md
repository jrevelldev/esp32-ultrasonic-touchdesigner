# Prova ESP32 → Wi-Fi → TouchDesigner

La distància es genera a l'ESP32: no cal cap sensor connectat per fer aquesta prova. TouchDesigner rep els missatges de l'ESP per Wi-Fi; no genera les lectures.

## Per provar-ho

1. Mantén l'ESP alimentat per USB.
2. Connecta l'ordinador al Wi-Fi de casa.
3. Obre el projecte **TouchDesigner/Prova_ESP32_TouchDesigner.toe**. La finestra amb la distància i el cercle s'obrirà automàticament.
4. Obre **http://192.168.5.50** al navegador.
5. En mode **Automàtic**, la distància recorre de 20 a 200 cm i torna, en un cicle d'uns 12 segons.
6. Per provar un valor concret, selecciona **Manual**, introdueix una distància entre 20 i 200 i prem **Aplica simulació**. TouchDesigner ha de mostrar el mateix valor i canviar la mida del cercle.

El formulari conserva l'estètica de la versió anterior. Afegeix un apartat **TouchDesigner · OSC** amb la destinació de les lectures.

## Adreces de la prova

| Element | Valor |
| --- | --- |
| Web de l'ESP, dins el Wi-Fi de casa | http://192.168.5.50 |
| IP del Mac en fer la prova | 192.168.5.179 |
| Port de recepció OSC de TouchDesigner | UDP 9000 |
| Xarxa de configuració de l'ESP | ESP-Config-<identificador de la placa> |
| Contrasenya del portal | Configura32 |
| Portal quan estàs connectat a la xarxa de configuració | http://192.168.4.1 |

Si la IP del Mac canvia, posa la nova IP al camp **IP de l'ordinador amb TouchDesigner** de la web de l'ESP i prem **Desa destinació OSC**. Aquest camp no és la IP de l'ESP.

La destinació OSC, el port i la configuració Wi-Fi/IP es conserven en apagar. El mode manual és temporal: en reiniciar, la simulació torna a automàtic.

Cada placa genera el seu nom de configuració a partir de la MAC de fàbrica completa, sense els dos punts: per exemple, **ESP-Config-8CD0B2D07415**. El nom és estable entre reinicis i es mostra a la web i al monitor sèrie. Es pot carregar el mateix programa a diverses plaques sense editar-ne el nom. Els camps `device_id` i `config_ssid` de `/telemetry` permeten consultar-lo també des d'un altre programa.

El nom individual no canvia la IP del portal: cada placa continua utilitzant **192.168.4.1** quan t'hi connectes directament. Dins la xarxa de casa, cada dispositiu ha de tenir una IP diferent.

## Senyals del LED

- Encès fix: connectat al Wi-Fi, amb el portal tancat.
- Parpelleig lent: intentant connectar.
- Parpelleig regular: portal de configuració obert.
- Parpelleig molt ràpid mentre prems BOOT: ja han passat 5 segons; deixa'l anar.

RESET reinicia sense esborrar la configuració. BOOT durant 5 segons, deixant-lo anar després, torna a obrir el portal.

## Què rep TouchDesigner

El component `esp32_demo` conté aquests nodes per utilitzar-los en altres visuals:

| Node | Valor |
| --- | --- |
| `osc_in` | Tots els missatges rebuts al port 9000 |
| `distancia_cm` | Distància en centímetres |
| `proximitat` | De 0 (lluny, 200 cm) a 1 (a prop, 20 cm) |
| `sequencia` | Comptador que permet detectar si continuen arribant lectures |
| `dashboard` | Finestra de prova amb la distància i el cercle |

Missatges: `/sensor/distancia` (float), `/sensor/proximitat` (float), `/sensor/simulat` (int, 1 durant aquesta prova), `/sensor/seq` (int). Es transmeten aproximadament 10 lectures per segon.

Si no arriba cap seqüència nova durant 1,5 segons, la finestra mostra que està esperant l'ESP i deixa de presentar la distància antiga com a activa.

## Fitxers

- `ESP32_OSC_Demo/ESP32_OSC_Demo.ino`: programa complet carregat a la placa.
- `TouchDesigner/Prova_ESP32_TouchDesigner.toe`: projecte complet.
- `TouchDesigner/Prova_ESP32_TouchDesigner.tox`: component reutilitzable per importar-lo en un altre projecte.
- `ESP32_Config/ESP32_Config.ino`: versió anterior, sense enviament OSC, conservada com a referència.

La xarxa té diversos punts d'accés amb el mateix nom. La versió de prova busca tots els canals i prioritza el senyal més fort. També conserva la potència reduïda de 8,5 dBm que ha funcionat amb aquesta placa.

## Per continuar amb el sensor

El pas següent és substituir el generador de distància simulat per la lectura del sensor, mantenint els mateixos missatges OSC i la mateixa web. Cal adaptar el senyal ECHO de 5 V a 3,3 V abans de connectar un HC-SR04 de 5 V a l'ESP32. El cablejat queda pendent.

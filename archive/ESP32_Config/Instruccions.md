# ESP32-C3: Wi-Fi, IP fixa i LED

Obre `ESP32_Config.ino` amb Arduino IDE. Selecciona **ESP32C3 Dev Module**, el port USB de la placa i **USB CDC On Boot → Enabled**. Carrega el programa amb la fletxa. No cal instal·lar biblioteques addicionals.

## Primera configuració

Aquesta versió utilitza un formulari propi i desa les dades separadament de la versió anterior. La primera vegada hauràs de tornar a introduir el Wi-Fi.

1. Connecta el mòbil a **ESP-Config**, amb contrasenya **Configura32**.
2. Si el formulari no s'obre sol, entra a **http://192.168.4.1**.
3. Escriu el nom exacte de la xarxa Wi-Fi de 2,4 GHz i la contrasenya.
4. Per a la primera prova, deixa la IP buida. Prem **Desa i connecta**.
5. Quan connecti, la pàgina i el monitor sèrie mostraran la IP. El portal es tancarà aproximadament 20 segons després de connectar.
6. Torna al Wi-Fi de casa i obre **http://IP-DE-L-ESP** al navegador.

## Triar una IP fixa

1. Mantén BOOT premut durant 5 segons. Quan el LED parpellegi molt ràpid, deixa'l anar.
2. Connecta't a **ESP-Config** i obre **http://192.168.4.1**.
3. Si no canvies de xarxa, deixa la contrasenya buida: conservarà la guardada.
4. Escriu una IP lliure de la xarxa de casa. Els camps de router, màscara i DNS s'omplen amb les dades actuals si l'ESP està connectat; comprova'ls abans de desar.
5. Prem **Desa i connecta**. Torna al Wi-Fi de casa i obre **http://IP-QUE-HAS-TRIAT**.

La IP fixa i el Wi-Fi es guarden i es conserven en apagar o reiniciar. Per tornar a IP automàtica, esborra el camp IP i desa. Si el DNS és buit però hi ha IP fixa, s'utilitza la porta d'enllaç com a DNS.

La IP ha d'estar lliure i cal excloure-la del rang DHCP del router o reservar-la per a aquest dispositiu. El formulari comprova el format i la subxarxa, però no pot garantir que no hi hagi un altre aparell amb aquesta IP.

El portal té una adreça pròpia: **192.168.4.1**. La IP fixa que tries és per al Wi-Fi de casa. Aquesta versió rebutja configuracions fixes que se superposin amb la subxarxa del portal; en aquest cas pots fer servir DHCP i una reserva al router.

## Senyals del LED programable

| Senyal | Significat |
| --- | --- |
| Encès fix | Connectat al Wi-Fi; portal tancat. |
| Parpelleig lent | Intentant connectar; portal tancat. |
| Parpelleig regular | Portal de configuració obert. |
| Parpelleig molt ràpid mantenint BOOT | Ja han passat 5 segons: deixa anar el botó. |

S'utilitza GPIO8, actiu a nivell baix, com a les Super Mini habituals. El LED d'alimentació pot continuar encès independentment d'aquests senyals.

RESET reinicia sense esborrar les dades. Si la connexió falla durant aproximadament 30 segons, s'obre el portal. BOOT també permet recuperar la configuració encara que la IP fixada no permeti accedir a la web. No premis RESET mentre mantens BOOT: aquesta combinació és per carregar programes.

Es manté la potència Wi-Fi reduïda a 8,5 dBm que ha funcionat amb aquesta placa. La web encara mostra **Sensor pendent de connectar**.

Validació: compilació per a ESP32-C3 amb USB CDC activat; el funcionament del nou formulari i del LED s'ha de comprovar a la placa.

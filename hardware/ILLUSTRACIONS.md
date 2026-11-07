# Referències i il·lustracions · 7 de novembre de 2026

Les tres captures aportades per l’usuari es conserven sense modificar a `referencies/`: frontal i posterior de l’ESP32-C3 Super Mini i frontal del HC-SR04.

La il·lustració principal s’ha refet amb l’eina integrada de generació d’imatges, utilitzant aquestes referències. Conserva el sensor en vertical per preferència de l’usuari. No és una fotografia d’un muntatge físic validat. La versió escollida és `docs/assets/connexio-illustrada.png`; les dues il·lustracions inicials es conserven només com a històric a `archive/illustracions-inicials/`.

## Especificació utilitzada per a la imatge final

Il·lustració educativa realista, en català, amb l’ESP32-C3 Super Mini a l’esquerra vist de davant, USB a dalt, BOOT/RST sota el connector i antena a baix. Costat dret, de dalt a baix: 5V, GND, 3V3, GPIO4, GPIO3, GPIO2, GPIO1 i GPIO0. Sensor HC-SR04 a la dreta, girat 90° en sentit horari respecte de la foto frontal, amb les dues càpsules en vertical i pins cap a l’esquerra: VCC, TRIG, ECHO, GND de dalt a baix.

Connexions: vermell 5V → VCC; blau GPIO4 → TRIG; groc ECHO → R1 2,2 kΩ → punt A; verd punt A → GPIO3; punt A → R2 3,3 kΩ → GND. El GND de l’ESP és el segon pin dret i ha d’estar unit al GND del sensor. La part inferior de R2 ha d’arribar visiblement al mateix terminal GND del sensor. No hi ha cap connexió a GPIO0 ni al pin 3V3. Resistències representades com a símbols rectangulars per evitar bandes de color equívoques. Punt A ≈3,0 V. Títol «ESP32-C3 Super Mini + HC-SR04», subtítol «Sensor a 5 V · ECHO adaptat a ≈3 V».

Les iteracions han corregit explícitament la continuïtat de GPIO4 a TRIG, l’absència de connexió a GPIO0 i el punt comú de les dues branques de GND. El circuit també es presenta en SVG editable a `esquema.svg`, amb fletxes de corrent convencional quan ECHO és alt.

## Alimentació

Les imatges mostren el muntatge de referència a **5 V amb divisor**. La foto «HC-SR04» no acredita una variant de 3,3 V. L’alternativa d’un sensor especificat per a 3,3 V s’explica a la guia, però no es presenta com un muntatge validat del sensor de les captures.

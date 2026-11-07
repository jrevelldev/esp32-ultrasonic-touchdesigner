#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <WiFiUdp.h>
#include "esp_mac.h"

// Super Mini habitual: LED a GPIO8, actiu a nivell baix.
const int LED_PIN = 8;
const int CONFIG_BUTTON_PIN = 9;
char AP_NAME[32] = "";
char DEVICE_ID[13] = "";
const char* AP_PASSWORD = "Configura32";
const uint32_t CONFIG_VERSION = 1;

struct Configuration {
  uint32_t version;
  char ssid[33];
  char password[65];
  char ip[16];
  char gateway[16];
  char mask[16];
  char dns[16];
};

Configuration config = {};
Preferences preferences;
WebServer server(80);
DNSServer dnsServer;
bool storageReady = false;
bool portal = false;
bool applyPending = false;
bool connecting = false;
bool connectionFailed = false;
bool wasConnected = false;
bool previousButton = false;
bool longPress = false;
unsigned long pressStarted = 0;
unsigned long connectionStarted = 0;
unsigned long lastConnected = 0;
unsigned long closePortalAt = 0;
unsigned long lastLedChange = 0;
int previousLedPeriod = -1;

WiFiUDP oscUdp;
String oscHost = ""; // Configura la IP del teu ordinador des de la web.
uint16_t oscPort = 9000;
bool automaticSimulation = true;
float distanceCm = 110.0f;
float manualDistanceCm = 110.0f;
float proximityValue = 0.5f;
uint32_t oscSequence = 0;
uint32_t oscFramesSent = 0;
unsigned long lastOscSend = 0;

void initializeDeviceIdentity() {
  uint8_t mac[6];
  if (esp_efuse_mac_get_default(mac) != ESP_OK) {
    Serial.println("No s'ha pogut llegir l'identificador de la placa.");
    return;
  }
  snprintf(DEVICE_ID, sizeof(DEVICE_ID), "%02X%02X%02X%02X%02X%02X",
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  snprintf(AP_NAME, sizeof(AP_NAME), "ESP-Config-%s", DEVICE_ID);
  Serial.print("Dispositiu: ");
  Serial.println(DEVICE_ID);
  Serial.print("Xarxa de configuracio: ");
  Serial.println(AP_NAME);
}

String escapeHtml(String value) {
  value.replace("&", "&amp;");
  value.replace("<", "&lt;");
  value.replace(">", "&gt;");
  value.replace("\"", "&quot;");
  value.replace("'", "&#39;");
  return value;
}

bool parseIP(const String& value, IPAddress& address) {
  int dots = 0;
  for (unsigned int i = 0; i < value.length(); i++) {
    char c = value[i];
    if (c == '.') dots++;
    else if (c < '0' || c > '9') return false;
  }
  return dots == 3 && address.fromString(value);
}

uint32_t ipNumber(const IPAddress& address) {
  return (uint32_t(address[0]) << 24) |
         (uint32_t(address[1]) << 16) |
         (uint32_t(address[2]) << 8) | address[3];
}

String validateAddresses(String ip, String gateway, String mask, String dns) {
  if (ip.isEmpty()) return ""; // IP buida = DHCP.
  IPAddress a, g, m, d;
  if (!parseIP(ip, a) || !parseIP(gateway, g) ||
      !parseIP(mask, m) || !parseIP(dns, d)) {
    return "Revisa IP, porta d'enllaç, màscara i DNS: calen adreces IPv4 completes.";
  }
  uint32_t bits = ipNumber(m);
  uint32_t hosts = ~bits;
  if (bits == 0 || hosts < 3 || (hosts & (hosts + 1)) != 0) {
    return "La màscara no és vàlida per a una xarxa local amb diversos dispositius.";
  }
  uint32_t hostA = ipNumber(a) & hosts;
  uint32_t hostG = ipNumber(g) & hosts;
  if (a[0] == 0 || a[0] == 127 || a[0] >= 224 ||
      g[0] == 0 || g[0] == 127 || g[0] >= 224 ||
      d[0] == 0 || d[0] == 127 || d[0] >= 224 ||
      hostA == 0 || hostA == hosts || hostG == 0 || hostG == hosts ||
      ipNumber(a) == ipNumber(g) ||
      (ipNumber(a) & bits) != (ipNumber(g) & bits)) {
    return "La IP i el router han de ser adreces diferents i vàlides de la mateixa subxarxa.";
  }
  // Evita superposar la xarxa domèstica amb la del portal.
  IPAddress portalIP(192, 168, 4, 1);
  if ((ipNumber(a) & bits) == (ipNumber(portalIP) & bits)) {
    return "Aquesta subxarxa se superposa amb el portal 192.168.4.1. Deixa la IP buida per usar DHCP.";
  }
  return "";
}

String inputField(const char* label, const char* name, String value, int limit) {
  return String("<label>") + label + "<input id='" + name + "' name='" + name +
    "' maxlength='" + String(limit) + "' value='" + escapeHtml(value) + "'></label>";
}

String pageHead(const char* title) {
  return String("<!doctype html><html lang='ca'><meta charset='utf-8'>") +
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>" + title + "</title><style>"
    "body{font:17px system-ui;background:#eef3f8;color:#18334d;max-width:620px;margin:30px auto;padding:18px}"
    "main{background:white;border-radius:18px;padding:26px}label{display:block;margin:14px 0}"
    "input,select,button{box-sizing:border-box;width:100%;padding:12px;font:inherit;border:1px solid #aab9c8;border-radius:8px}"
    "button{background:#165bcc;color:white;cursor:pointer;margin-top:12px}small{display:block;color:#52677a}"
    ".error{color:#a11}a{color:#165bcc}</style><main><h1>" + title + "</h1>";
}

void handleWifiScan() {
  static bool scanning = false;
  server.sendHeader("Cache-Control", "no-store");
  if (!portal) {
    server.send(403, "text/plain", "Obre el mode configuracio amb BOOT.");
    return;
  }
  if (!scanning) {
    WiFi.scanDelete();
    int result = WiFi.scanNetworks(true, false);
    if (result == WIFI_SCAN_FAILED) {
      server.send(503, "text/plain", "No s'ha pogut iniciar la cerca.");
      return;
    }
    scanning = true;
  }
  int count = WiFi.scanComplete();
  if (count == WIFI_SCAN_RUNNING) {
    server.send(202, "text/plain", "Buscant xarxes...");
    return;
  }
  scanning = false;
  if (count < 0) {
    WiFi.scanDelete();
    server.send(503, "text/plain", "La cerca ha fallat. Torna-ho a provar.");
    return;
  }
  String options = "<option value='' disabled selected>Tria una xarxa...</option>";
  int visible = 0;
  for (int i = 0; i < count; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.isEmpty()) continue;
    bool duplicate = false;
    for (int j = 0; j < i; j++) {
      if (WiFi.SSID(j) == ssid) { duplicate = true; break; }
    }
    if (duplicate) continue;
    String escaped = escapeHtml(ssid);
    options += "<option value='" + escaped + "'>" + escaped +
      " (" + String(WiFi.RSSI(i)) + " dBm)</option>";
    visible++;
  }
  if (!visible) options = "<option value='' disabled selected>No s'ha trobat cap xarxa</option>";
  WiFi.scanDelete();
  server.send(200, "text/html; charset=utf-8", options);
}

String networkPicker() {
  return R"HTML(
<label>Xarxes Wi-Fi disponibles
<select id="wifi-list" onchange="document.getElementById('ssid').value=this.value">
<option value="" disabled selected>Buscant xarxes...</option>
</select></label>
<button id="scan-button" type="button" onclick="scanWifi()">Torna a buscar</button>
<small id="scan-state" role="status">Buscant xarxes de 2,4 GHz...</small>
<script>
async function scanWifi() {
  const button = document.getElementById('scan-button');
  const state = document.getElementById('scan-state');
  const list = document.getElementById('wifi-list');
  button.disabled = true;
  list.disabled = true;
  state.textContent = 'Buscant xarxes de 2,4 GHz...';
  try {
    for (let attempt = 0; attempt < 25; attempt++) {
      const response = await fetch('/wifi-scan', {cache:'no-store'});
      if (response.status === 202) {
        await new Promise(resolve => setTimeout(resolve, 1000));
        continue;
      }
      if (!response.ok) throw new Error('scan');
      list.innerHTML = await response.text();
      const current = document.getElementById('ssid').value;
      if (Array.from(list.options).some(option => option.value === current)) {
        list.value = current;
      }
      state.textContent = 'Tria una xarxa o escriu-ne el nom al camp de sota.';
      return;
    }
    throw new Error('timeout');
  } catch (error) {
    state.textContent = 'No s’ha pogut completar la cerca. Prem Torna a buscar o escriu el nom manualment.';
  } finally {
    button.disabled = false;
    list.disabled = false;
  }
}
window.addEventListener('DOMContentLoaded', scanWifi);
</script>
)HTML";
}

void sendConfigPage(String error = "", int status = 200) {
  String html = pageHead("Configura el teu ESP32");
  html += "<p><small>Dispositiu: <strong>" + String(DEVICE_ID) +
    "</strong><br>Xarxa de configuració: " + String(AP_NAME) + "</small></p>";
  if (!error.isEmpty()) html += "<p class='error'>" + escapeHtml(error) + "</p>";
  html += "<p>El LED parpelleja mentre aquest mode és obert.</p>";
  if (WiFi.status() == WL_CONNECTED) {
    html += "<p>Connexió actual: <strong>" + WiFi.localIP().toString() +
      "</strong><br>Router: " + WiFi.gatewayIP().toString() +
      "<br>Màscara: " + WiFi.subnetMask().toString() +
      "<br>DNS: " + WiFi.dnsIP().toString() + "</p>";
  }
  html += "<form method='post' action='/save'>";
  html += networkPicker();
  html += inputField("Xarxa seleccionada (o nom d'una xarxa oculta)", "ssid", config.ssid, 32);
  html += "<label>Contrasenya Wi-Fi<input name='password' type='password' maxlength='64' autocomplete='new-password'></label>"
    "<small>Si no canvies de xarxa, deixa-la buida per conservar la contrasenya guardada.</small>"
    "<h2>Adreça de xarxa</h2><p>Deixa la IP buida per obtenir-la automàticament (DHCP).</p>";
  html += inputField("IP fixa (opcional)", "ip", config.ip, 15);
  String gateway = config.gateway;
  String mask = config.mask;
  String dns = config.dns;
  if (WiFi.status() == WL_CONNECTED) {
    if (gateway.isEmpty()) gateway = WiFi.gatewayIP().toString();
    if (mask.isEmpty()) mask = WiFi.subnetMask().toString();
    if (dns.isEmpty()) dns = WiFi.dnsIP().toString();
  }
  html += inputField("Porta d'enllaç (IP del router)", "gateway", gateway, 15);
  html += inputField("Màscara de subxarxa", "mask", mask, 15);
  html += inputField("DNS (si el deixes buit, s'utilitza el router)", "dns", dns, 15);
  html += "<small>Tria una IP lliure i exclou-la del rang automàtic del router, o reserva-la per a aquest ESP.</small>"
    "<button>Desa i connecta</button></form>";
  if (WiFi.status() == WL_CONNECTED) {
    html += "<form method='post' action='/finish'><button>Tanca la configuració</button></form>";
  }
  html += oscConfigPanel();
  html += "</main></html>";
  server.send(status, "text/html; charset=utf-8", html);
}


// Missatges OSC 1.0: text acabat en zero, alineat a 4 bytes,
// seguit d'una etiqueta de tipus i un valor de 32 bits big-endian.
bool sendOscValue(const char* address, char type, uint32_t bits) {
  IPAddress destination;
  if (!destination.fromString(oscHost)) return false;
  uint8_t packet[96] = {};
  size_t length = strlen(address) + 1;
  if (length > 72) return false;
  memcpy(packet, address, length);
  size_t offset = (length + 3) & ~size_t(3);
  packet[offset] = ',';
  packet[offset + 1] = type;
  offset += 4;
  packet[offset++] = (bits >> 24) & 255;
  packet[offset++] = (bits >> 16) & 255;
  packet[offset++] = (bits >> 8) & 255;
  packet[offset++] = bits & 255;
  if (!oscUdp.beginPacket(destination, oscPort)) return false;
  oscUdp.write(packet, offset);
  return oscUdp.endPacket() == 1;
}

bool sendOscFloat(const char* address, float value) {
  uint32_t bits;
  memcpy(&bits, &value, sizeof(bits));
  return sendOscValue(address, 'f', bits);
}

void updateSimulationAndOsc() {
  if (automaticSimulation) {
    float phase = float(millis() % 12000UL) / 6000.0f;
    distanceCm = 20.0f + 180.0f * (phase <= 1.0f ? phase : 2.0f - phase);
  } else distanceCm = manualDistanceCm;
  proximityValue = constrain((200.0f - distanceCm) / 180.0f, 0.0f, 1.0f);
  if (WiFi.status() != WL_CONNECTED || millis() - lastOscSend < 100) return;
  lastOscSend = millis();
  oscSequence++;
  bool ok = sendOscFloat("/sensor/distancia", distanceCm);
  ok = sendOscFloat("/sensor/proximitat", proximityValue) && ok;
  ok = sendOscValue("/sensor/simulat", 'i', 1) && ok;
  ok = sendOscValue("/sensor/seq", 'i', oscSequence) && ok;
  if (ok) oscFramesSent++;
  static unsigned long lastReport = 0;
  if (millis() - lastReport >= 5000) {
    lastReport = millis();
    Serial.printf("OSC: %.1f cm -> %s:%u | enviades=%lu | RSSI=%d | potencia=%d\n",
      distanceCm, oscHost.c_str(), oscPort, (unsigned long)oscFramesSent,
      WiFi.RSSI(), (int)WiFi.getTxPower());
  }
}

String oscConfigPanel() {
  String html = "<h2>TouchDesigner · OSC</h2><form method='post' action='/osc'>";
  html += inputField("IP de l'ordinador amb TouchDesigner", "osc_host", oscHost, 15);
  html += inputField("Port OSC", "osc_port", String(oscPort), 5);
  html += "<small>Aquesta és la IP del Mac, no la de l'ESP. OSC In CHOP ha d'escoltar el mateix port.</small>";
  html += "<button>Desa destinació OSC</button></form>";
  return html;
}

String simulationPanel() {
  String html = R"HTML(
<p><strong>PROVA · SENSOR SIMULAT</strong></p>
<p id="reading" style="font-size:48px;font-weight:750;margin:12px 0">— cm</p>
<progress id="proximity" max="1" value="0" style="width:100%;height:22px"></progress>
<p id="telemetry-state">Esperant la lectura…</p>
<form method="post" action="/simulation">
<label>Mode de prova<select name="mode">
)HTML";
  html += String("<option value='auto'") + (automaticSimulation ? " selected" : "") + ">Automàtic · 20–200 cm</option>";
  html += String("<option value='manual'") + (!automaticSimulation ? " selected" : "") + ">Manual · valor fix</option></select></label>";
  html += "<label>Distància manual (cm)<input type='number' name='distance' min='20' max='200' step='0.1' value='" + String(manualDistanceCm, 1) + "'></label>";
  html += R"HTML(
<button>Aplica simulació</button></form>
<script>
setInterval(async()=>{try{
 const t=await(await fetch('/telemetry',{cache:'no-store'})).json();
 document.getElementById('reading').textContent=t.distance_cm.toFixed(1)+' cm';
 document.getElementById('proximity').value=t.proximity;
 document.getElementById('telemetry-state').textContent=
   (t.automatic?'Moviment automàtic':'Valor manual')+' · OSC → '+t.osc_host+':'+t.osc_port+
   ' · '+t.frames_sent+' lectures enviades';
}catch(e){}},300);
</script>
)HTML";
  return html;
}

void saveOscDestination() {
  String host = server.arg("osc_host"); host.trim();
  String portText = server.arg("osc_port"); portText.trim();
  IPAddress destination;
  bool digits = !portText.isEmpty();
  for (unsigned int i = 0; i < portText.length(); i++) if (!isDigit(portText[i])) digits = false;
  long port = portText.toInt();
  if (!parseIP(host, destination) || destination[0] == 0 || destination[0] >= 224 ||
      !digits || port < 1024 || port > 65535) {
    server.send(400, "text/html; charset=utf-8", pageHead("Revisa la destinació") +
      "<p>Cal una IP d'ordinador vàlida i un port entre 1024 i 65535.</p><a href='/'>Torna</a></main></html>");
    return;
  }
  if (!storageReady) { server.send(500, "text/plain", "No es pot guardar."); return; }
  oscHost = host;
  oscPort = uint16_t(port);
  preferences.putString("osc-host", oscHost);
  preferences.putUShort("osc-port", oscPort);
  server.sendHeader("Location", "/");
  server.send(303, "text/plain", "");
}

void setSimulation() {
  String mode = server.arg("mode");
  String raw = server.arg("distance");
  char* end;
  float value = strtof(raw.c_str(), &end);
  if ((mode != "auto" && mode != "manual") || raw.isEmpty() || *end != '\0' ||
      !isfinite(value) || value < 20 || value > 200) {
    server.send(400, "text/plain", "La distancia ha d'estar entre 20 i 200 cm."); return;
  }
  automaticSimulation = mode == "auto";
  manualDistanceCm = value;
  server.sendHeader("Location", "/");
  server.send(303, "text/plain", "");
}

void sendTelemetry() {
  String json = "{\"simulated\":true,\"automatic\":";
  json += automaticSimulation ? "true" : "false";
  json += ",\"distance_cm\":" + String(distanceCm, 2);
  json += ",\"proximity\":" + String(proximityValue, 4);
  json += ",\"sequence\":" + String(oscSequence);
  json += ",\"frames_sent\":" + String(oscFramesSent);
  json += ",\"osc_host\":\"" + oscHost + "\",\"osc_port\":" + String(oscPort);
  json += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
  json += ",\"device_id\":\"" + String(DEVICE_ID) + "\",\"config_ssid\":\"" + String(AP_NAME) + "\"}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void homePage() {
  if (portal) { sendConfigPage(); return; }
  String html = pageHead("El meu ESP32 · distància");
  html += "<p><small>Dispositiu: <strong>" + String(DEVICE_ID) +
    "</strong><br>Xarxa de configuració: " + String(AP_NAME) + "</small></p>";
  html += simulationPanel();
  html += "<p>IP de l'ESP: <strong>" + WiFi.localIP().toString() + "</strong></p>";
  html += oscConfigPanel();
  html += "<p>LED fix: connectat. Mantén BOOT 5 segons i deixa'l anar per configurar el Wi-Fi i la IP.</p></main></html>";
  server.send(200, "text/html; charset=utf-8", html);
}

void saveConfiguration() {
  if (!portal) { server.send(403, "text/plain", "Obre el mode configuracio amb BOOT."); return; }
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  String ip = server.arg("ip"); ip.trim();
  String gateway = server.arg("gateway"); gateway.trim();
  String mask = server.arg("mask"); mask.trim();
  String dns = server.arg("dns"); dns.trim();
  if (ssid.isEmpty() || ssid.length() > 32 || password.length() > 64) {
    sendConfigPage("Revisa el nom i la contrasenya de la xarxa.", 400); return;
  }
  if (password.isEmpty() && ssid == String(config.ssid)) password = config.password;
  if (!ip.isEmpty() && dns.isEmpty()) dns = gateway;
  String error = validateAddresses(ip, gateway, mask, dns);
  if (!error.isEmpty()) { sendConfigPage(error, 400); return; }
  if (ip.isEmpty()) { gateway = ""; mask = ""; dns = ""; }
  Configuration next = {};
  next.version = CONFIG_VERSION;
  ssid.toCharArray(next.ssid, sizeof(next.ssid));
  password.toCharArray(next.password, sizeof(next.password));
  ip.toCharArray(next.ip, sizeof(next.ip));
  gateway.toCharArray(next.gateway, sizeof(next.gateway));
  mask.toCharArray(next.mask, sizeof(next.mask));
  dns.toCharArray(next.dns, sizeof(next.dns));
  if (!storageReady || preferences.putBytes("settings", &next, sizeof(next)) != sizeof(next)) {
    sendConfigPage("No s'ha pogut guardar la configuració. Torna-ho a provar.", 500); return;
  }
  config = next;
  String html = pageHead("Configuració guardada");
  html += R"HTML(
<p id="state">Connectant al router… Pot trigar 30 segons.</p>
<p id="address"></p><p>Torna al Wi-Fi de casa per obrir la web.</p>
<p><a href="/">Torna al formulari</a></p>
<script>
setInterval(async()=>{try{
const s=await(await fetch('/status',{cache:'no-store'})).json();
document.getElementById('state').textContent=s.connected?
'Connectat! La xarxa de configuració es tancarà en uns segons.':
(s.failed?'No s’ha pogut connectar. Torna al formulari i revisa les dades.':'Connectant…');
document.getElementById('address').textContent=s.connected?'Web: http://'+s.ip:'';
}catch(e){}},1500);
</script></main></html>
)HTML";
  server.send(200, "text/html; charset=utf-8", html);
  applyPending = true;
}

void reducePower() {
  if (!WiFi.setTxPower(WIFI_POWER_8_5dBm)) Serial.println("No s'ha pogut ajustar la potencia.");
}

void connectToRouter() {
  closePortalAt = 0;
  connectionFailed = false;
  wasConnected = false;
  WiFi.disconnect(false, false);
  WiFi.mode(portal ? WIFI_AP_STA : WIFI_STA);
  reducePower();
  IPAddress zero(0, 0, 0, 0);
  bool ok;
  if (config.ip[0]) {
    IPAddress ip, gateway, mask, dns;
    ip.fromString(config.ip); gateway.fromString(config.gateway);
    mask.fromString(config.mask); dns.fromString(config.dns);
    ok = WiFi.config(ip, gateway, mask, dns);
  } else {
    ok = WiFi.config(zero, zero, zero);
  }
  connecting = ok;
  connectionStarted = millis();
  if (ok) {
    WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
    WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
    WiFi.begin(config.ssid, config.password);
    Serial.println("Connectant al Wi-Fi guardat...");
  } else {
    connectionFailed = true;
    Serial.println("Error aplicant la configuracio IP.");
  }
}

void openPortal() {
  if (portal) return;
  if (!AP_NAME[0]) { Serial.println("No es pot obrir el portal sense identificador."); return; }
  WiFi.mode(WIFI_AP_STA);
  reducePower();
  IPAddress ip(192, 168, 4, 1), mask(255, 255, 255, 0);
  bool ok = WiFi.softAPConfig(ip, ip, mask);
  ok = WiFi.softAP(AP_NAME, AP_PASSWORD, 1, 0, 4) && ok;
  reducePower();
  if (!ok) { Serial.println("Error obrint ESP-Config."); return; }
  portal = true;
  dnsServer.start(53, "*", ip);
  server.begin();
  Serial.print("CONFIGURACIO OBERTA: ");
  Serial.println(AP_NAME);
  Serial.print("Contrasenya: ");
  Serial.println(AP_PASSWORD);
  Serial.println("Portal: http://192.168.4.1");
}

void closePortal() {
  dnsServer.stop();
  WiFi.softAPdisconnect(false);
  WiFi.mode(WIFI_STA);
  reducePower();
  portal = false;
  closePortalAt = 0;
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.setSleep(false);
    server.begin();
    oscUdp.stop();
    oscUdp.begin(0);
  }
  Serial.print("Configuracio tancada. WEB DISPONIBLE: http://");
  Serial.println(WiFi.localIP());
}

void updateLed() {
  int period = longPress ? 80 : portal ? 250 :
    WiFi.status() == WL_CONNECTED ? 0 : 800;
  if (period != previousLedPeriod) {
    previousLedPeriod = period;
    lastLedChange = millis();
    digitalWrite(LED_PIN, LOW);
  }
  if (period > 0 && millis() - lastLedChange >= (unsigned long)period) {
    lastLedChange = millis();
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
}

void setup() {
  digitalWrite(LED_PIN, HIGH);
  pinMode(LED_PIN, OUTPUT);
  pinMode(CONFIG_BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  delay(1500);
  initializeDeviceIdentity();
  WiFi.onEvent(
    [](WiFiEvent_t event, WiFiEventInfo_t info) {
      Serial.print("\nMotiu de desconnexio: ");
      Serial.println(info.wifi_sta_disconnected.reason);
    },
    ARDUINO_EVENT_WIFI_STA_DISCONNECTED
  );
  storageReady = preferences.begin("esp-config", false);
  if (storageReady && preferences.getBytesLength("settings") == sizeof(config)) {
    preferences.getBytes("settings", &config, sizeof(config));
    if (config.version != CONFIG_VERSION) config = {};
  }
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
  reducePower();
  if (storageReady) {
    oscHost = preferences.getString("osc-host", "");
    oscPort = preferences.getUShort("osc-port", 9000);
  }
  oscUdp.begin(0);
  server.on("/", HTTP_GET, homePage);
  server.on("/save", HTTP_POST, saveConfiguration);
  server.on("/wifi-scan", HTTP_GET, handleWifiScan);
  server.on("/osc", HTTP_POST, saveOscDestination);
  server.on("/simulation", HTTP_POST, setSimulation);
  server.on("/telemetry", HTTP_GET, sendTelemetry);
  server.on("/status", HTTP_GET, []() {
    bool connected = !applyPending && WiFi.status() == WL_CONNECTED;
    String json = String("{\"connected\":") + (connected ? "true" : "false") +
      ",\"failed\":" + (connectionFailed ? "true" : "false") +
      ",\"ip\":\"" + WiFi.localIP().toString() + "\"}";
    server.sendHeader("Cache-Control", "no-store");
    server.send(200, "application/json", json);
  });
  server.on("/finish", HTTP_POST, []() {
    if (!portal || WiFi.status() != WL_CONNECTED) {
      server.send(409, "text/plain", "Cal estar connectat al router."); return;
    }
    server.send(200, "text/html; charset=utf-8", pageHead("Tancant configuració") +
      "<p>Torna al Wi-Fi de casa i obre http://" + WiFi.localIP().toString() + "</p></main></html>");
    closePortalAt = millis() + 3000;
  });
  server.onNotFound([]() {
    if (portal) {
      server.sendHeader("Location", "http://192.168.4.1/");
      server.send(302, "text/plain", "");
    } else server.send(404, "text/plain", "Pagina no trobada");
  });
  server.begin();
  lastConnected = millis();
  if (config.ssid[0]) connectToRouter();
  else openPortal();
}

void loop() {
  server.handleClient();
  if (portal) dnsServer.processNextRequest();
  if (applyPending) { applyPending = false; connectToRouter(); }

  bool pressed = digitalRead(CONFIG_BUTTON_PIN) == LOW;
  if (pressed && !previousButton) { pressStarted = millis(); longPress = false; }
  if (pressed && !longPress && millis() - pressStarted >= 5000) {
    longPress = true;
    Serial.println("Deixa anar BOOT per obrir la configuracio.");
  }
  if (!pressed && previousButton && longPress) {
    longPress = false;
    closePortalAt = 0;
    openPortal();
  }
  previousButton = pressed;

  bool connected = WiFi.status() == WL_CONNECTED;
  if (connected) {
    lastConnected = millis();
    if (!wasConnected) {
      reducePower();
      WiFi.setSleep(false);
      server.begin();
      oscUdp.stop();
      oscUdp.begin(0);
      Serial.print("WEB DISPONIBLE: http://");
      Serial.println(WiFi.localIP());
      Serial.printf("XARXA: IP=%s mascara=%s router=%s BSSID=%s\n",
        WiFi.localIP().toString().c_str(), WiFi.subnetMask().toString().c_str(),
        WiFi.gatewayIP().toString().c_str(), WiFi.BSSIDstr().c_str());
      if (connecting && portal) closePortalAt = millis() + 20000;
    }
    connecting = false;
    connectionFailed = false;
  } else {
    closePortalAt = 0;
    if (connecting && millis() - connectionStarted >= 30000) {
      connecting = false;
      connectionFailed = true;
      Serial.println("No s'ha pogut connectar. Revisa les dades al portal.");
      openPortal();
    }
    if (!portal && !connecting && millis() - lastConnected >= 30000) openPortal();
  }
  wasConnected = connected;
  if (portal && closePortalAt != 0 && (long)(millis() - closePortalAt) >= 0) closePortal();
  updateLed();
  updateSimulationAndOsc();
  delay(5);
}

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

// Super Mini habitual: LED a GPIO8, actiu a nivell baix.
const int LED_PIN = 8;
const int CONFIG_BUTTON_PIN = 9;
const char* AP_NAME = "ESP-Config";
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
  html += "</main></html>";
  server.send(status, "text/html; charset=utf-8", html);
}

void homePage() {
  if (portal) { sendConfigPage(); return; }
  String html = pageHead("El meu ESP32");
  html += "<h2>Sensor pendent de connectar</h2><p>IP: <strong>" +
    WiFi.localIP().toString() + "</strong></p><p>Assignació: " +
    (config.ip[0] ? "IP fixa" : "automàtica (DHCP)") +
    "</p><p>LED fix: connectat. Mantén BOOT 5 segons; quan parpellegi molt ràpid, deixa'l anar per configurar.</p></main></html>";
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
'Connectat! La xarxa ESP-Config es tancarà en uns segons.':
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
    WiFi.begin(config.ssid, config.password);
    Serial.println("Connectant al Wi-Fi guardat...");
  } else {
    connectionFailed = true;
    Serial.println("Error aplicant la configuracio IP.");
  }
}

void openPortal() {
  if (portal) return;
  WiFi.mode(WIFI_AP_STA);
  reducePower();
  IPAddress ip(192, 168, 4, 1), mask(255, 255, 255, 0);
  bool ok = WiFi.softAPConfig(ip, ip, mask);
  ok = WiFi.softAP(AP_NAME, AP_PASSWORD, 1, 0, 4) && ok;
  reducePower();
  if (!ok) { Serial.println("Error obrint ESP-Config."); return; }
  portal = true;
  dnsServer.start(53, "*", ip);
  Serial.println("CONFIGURACIO OBERTA: ESP-Config / Configura32");
  Serial.println("Portal: http://192.168.4.1");
}

void closePortal() {
  dnsServer.stop();
  WiFi.softAPdisconnect(false);
  WiFi.mode(WIFI_STA);
  reducePower();
  portal = false;
  closePortalAt = 0;
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
  server.on("/", HTTP_GET, homePage);
  server.on("/save", HTTP_POST, saveConfiguration);
  server.on("/wifi-scan", HTTP_GET, handleWifiScan);
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
      Serial.print("WEB DISPONIBLE: http://");
      Serial.println(WiFi.localIP());
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
  delay(5);
}

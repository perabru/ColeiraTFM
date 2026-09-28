// ============================================================
// COLEIRA INTELIGENTE PARA GADO
// ESP32 + LOCALIZACAO VIA WIFI + MQTT HIVEMQ
// ============================================================

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ============================================================
// 1. WIFI
// ============================================================

const char* WIFI_SSID  = "Bruno 2G";
const char* WIFI_SENHA = "11235813";

// ============================================================
// 2. API DE GEOLOCALIZACAO
// ============================================================
//
// Para obter latitude/longitude usando redes WiFi,
// coloque uma chave da Google Geolocation API abaixo.
//

const char* GEO_API_KEY = "COLOQUE_SUA_API_KEY_AQUI";

// ============================================================
// 3. HIVEMQ
// ============================================================

const char* MQTT_SERVIDOR =
  "d5010d2de7ff4182bd09c7fde4c243e5.s1.eu.hivemq.cloud";

const int MQTT_PORTA = 8883;

const char* MQTT_USUARIO = "tfm";
const char* MQTT_SENHA   = "123456789";

// ============================================================
// 4. TOPICOS MQTT
// ============================================================

const char* TOPICO_LOCALIZACAO =
  "gado/coleira01/localizacao";

const char* TOPICO_BATERIA =
  "gado/coleira01/bateria";

const char* TOPICO_TELEMETRIA =
  "gado/coleira01/telemetria";

const char* TOPICO_STATUS =
  "gado/coleira01/status";

// ============================================================
// CLIENTES
// ============================================================

WiFiClientSecure wifiSecure;
PubSubClient mqtt(wifiSecure);

// ============================================================
// VARIAVEIS
// ============================================================

double latitude  = 0.0;
double longitude = 0.0;
double precisao  = 0.0;

unsigned long ultimoEnvio = 0;

// Enviar dados a cada 60 segundos
const unsigned long INTERVALO_ENVIO = 60000;

// ============================================================
// ID UNICO DA COLEIRA
// ============================================================

String obterIDESP32() {

  uint64_t chipID = ESP.getEfuseMac();

  char id[40];

  sprintf(
    id,
    "ESP32-GADO-%04X%08X",
    (uint16_t)(chipID >> 32),
    (uint32_t)chipID
  );

  return String(id);
}

// ============================================================
// CONECTAR AO WIFI
// ============================================================

void conectarWiFi() {

  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.println();
  Serial.println("=================================");
  Serial.println("CONECTANDO AO WIFI");
  Serial.println("=================================");

  Serial.print("Rede: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);

  WiFi.begin(
    WIFI_SSID,
    WIFI_SENHA
  );

  int tentativa = 0;

  while (
    WiFi.status() != WL_CONNECTED &&
    tentativa < 40
  ) {

    delay(500);

    Serial.print(".");

    tentativa++;
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("WiFi conectado!");

    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    Serial.print("Sinal: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

  } else {

    Serial.println("Falha ao conectar ao WiFi.");

  }
}

// ============================================================
// CONECTAR AO HIVEMQ
// ============================================================

void conectarMQTT() {

  if (mqtt.connected()) {
    return;
  }

  Serial.println();
  Serial.println("Conectando ao HiveMQ...");

  String clienteID =
    obterIDESP32();

  if (
    mqtt.connect(
      clienteID.c_str(),
      MQTT_USUARIO,
      MQTT_SENHA,
      TOPICO_STATUS,
      0,
      true,
      "offline"
    )
  ) {

    Serial.println("HiveMQ conectado!");

    mqtt.publish(
      TOPICO_STATUS,
      "online",
      true
    );

  } else {

    Serial.print("Erro MQTT: ");
    Serial.println(mqtt.state());

  }
}

// ============================================================
// OBTER LOCALIZACAO ATRAVES DAS REDES WIFI
// ============================================================

bool obterLocalizacaoWiFi() {

  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  Serial.println();
  Serial.println("=================================");
  Serial.println("PROCURANDO REDES WIFI");
  Serial.println("=================================");

  int quantidade =
    WiFi.scanNetworks();

  if (quantidade <= 0) {

    Serial.println(
      "Nenhuma rede WiFi encontrada."
    );

    WiFi.scanDelete();

    return false;
  }

  Serial.print("Redes encontradas: ");
  Serial.println(quantidade);

  DynamicJsonDocument doc(8192);

  doc["considerIp"] = true;

  JsonArray redes =
    doc.createNestedArray(
      "wifiAccessPoints"
    );

  int quantidadeEnviar =
    quantidade;

  if (quantidadeEnviar > 15) {
    quantidadeEnviar = 15;
  }

  for (
    int i = 0;
    i < quantidadeEnviar;
    i++
  ) {

    JsonObject rede =
      redes.createNestedObject();

    rede["macAddress"] =
      WiFi.BSSIDstr(i);

    rede["signalStrength"] =
      WiFi.RSSI(i);

    rede["channel"] =
      WiFi.channel(i);

    Serial.print(i + 1);

    Serial.print(" | ");

    Serial.print(
      WiFi.SSID(i)
    );

    Serial.print(" | RSSI: ");

    Serial.print(
      WiFi.RSSI(i)
    );

    Serial.print(" | MAC: ");

    Serial.println(
      WiFi.BSSIDstr(i)
    );
  }

  String json;

  serializeJson(
    doc,
    json
  );

  WiFi.scanDelete();

  // ==========================================================
  // GOOGLE GEOLOCATION API
  // ==========================================================

  String url =
    "https://www.googleapis.com/geolocation/v1/geolocate?key=";

  url += GEO_API_KEY;

  WiFiClientSecure clienteHTTPS;

  clienteHTTPS.setInsecure();

  HTTPClient https;

  if (
    !https.begin(
      clienteHTTPS,
      url
    )
  ) {

    Serial.println(
      "Erro ao iniciar HTTPS."
    );

    return false;
  }

  https.addHeader(
    "Content-Type",
    "application/json"
  );

  Serial.println(
    "Solicitando localizacao..."
  );

  int codigoHTTP =
    https.POST(json);

  if (codigoHTTP != 200) {

    Serial.print(
      "Erro da API: "
    );

    Serial.println(
      codigoHTTP
    );

    Serial.println(
      https.getString()
    );

    https.end();

    return false;
  }

  String resposta =
    https.getString();

  https.end();

  DynamicJsonDocument respostaJson(2048);

  DeserializationError erro =
    deserializeJson(
      respostaJson,
      resposta
    );

  if (erro) {

    Serial.println(
      "Erro ao processar JSON."
    );

    return false;
  }

  latitude =
    respostaJson["location"]["lat"];

  longitude =
    respostaJson["location"]["lng"];

  precisao =
    respostaJson["accuracy"];

  Serial.println();
  Serial.println(
    "LOCALIZACAO ENCONTRADA"
  );

  Serial.print("Latitude: ");
  Serial.println(
    latitude,
    6
  );

  Serial.print("Longitude: ");
  Serial.println(
    longitude,
    6
  );

  Serial.print("Precisao: ");
  Serial.print(precisao);
  Serial.println(" metros");

  return true;
}

// ============================================================
// PUBLICAR LOCALIZACAO
// ============================================================

void enviarLocalizacao() {

  DynamicJsonDocument doc(1024);

  doc["animal"] =
    "gado01";

  doc["coleira"] =
    obterIDESP32();

  doc["latitude"] =
    latitude;

  doc["longitude"] =
    longitude;

  doc["precisao_m"] =
    precisao;

  doc["wifi"] =
    WIFI_SSID;

  doc["rssi"] =
    WiFi.RSSI();

  String mensagem;

  serializeJson(
    doc,
    mensagem
  );

  mqtt.publish(
    TOPICO_LOCALIZACAO,
    mensagem.c_str(),
    true
  );

  Serial.println();
  Serial.println(
    "LOCALIZACAO ENVIADA:"
  );

  Serial.println(
    mensagem
  );
}

// ============================================================
// BATERIA
// ============================================================
//
// IMPORTANTE:
//
// O power bank conectado na USB-C fornece aproximadamente 5V.
//
// A ESP32 normalmente NAO recebe do power bank a porcentagem
// interna da bateria.
//
// Portanto:
// bateria_pct = -1
//
// Para ter porcentagem real utilize, por exemplo,
// MAX17048 ligado a uma bateria LiPo/Li-ion.
//
// ============================================================

float obterPorcentagemBateria() {

  return -1;
}

// ============================================================
// ENVIAR BATERIA
// ============================================================

void enviarBateria() {

  DynamicJsonDocument doc(512);

  doc["coleira"] =
    obterIDESP32();

  doc["fonte"] =
    "Power Bank USB-C";

  doc["alimentacao"] =
    true;

  float bateria =
    obterPorcentagemBateria();

  if (bateria >= 0) {

    doc["porcentagem"] =
      bateria;

  } else {

    doc["porcentagem"] =
      nullptr;

    doc["mensagem"] =
      "Porcentagem nao disponivel pela USB-C";

  }

  String mensagem;

  serializeJson(
    doc,
    mensagem
  );

  mqtt.publish(
    TOPICO_BATERIA,
    mensagem.c_str(),
    true
  );

  Serial.println();
  Serial.println(
    "BATERIA:"
  );

  Serial.println(
    mensagem
  );
}

// ============================================================
// TELEMETRIA COMPLETA
// ============================================================

void enviarTelemetria() {

  DynamicJsonDocument doc(1024);

  doc["animal"] =
    "gado01";

  doc["coleira"] =
    obterIDESP32();

  doc["latitude"] =
    latitude;

  doc["longitude"] =
    longitude;

  doc["precisao_m"] =
    precisao;

  doc["wifi"] =
    WiFi.SSID();

  doc["rssi"] =
    WiFi.RSSI();

  doc["ip"] =
    WiFi.localIP().toString();

  doc["alimentacao"] =
    "Power Bank USB-C";

  doc["tempo_ligado_s"] =
    millis() / 1000;

  String mensagem;

  serializeJson(
    doc,
    mensagem
  );

  mqtt.publish(
    TOPICO_TELEMETRIA,
    mensagem.c_str(),
    true
  );

  Serial.println();
  Serial.println(
    "TELEMETRIA:"
  );

  Serial.println(
    mensagem
  );
}

// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  delay(2000);

  Serial.println();
  Serial.println(
    "================================="
  );

  Serial.println(
    " COLEIRA INTELIGENTE - GADO"
  );

  Serial.println(
    " ESP32 + WIFI + MQTT"
  );

  Serial.println(
    "================================="
  );

  conectarWiFi();

  // TLS HiveMQ

  wifiSecure.setInsecure();

  mqtt.setServer(
    MQTT_SERVIDOR,
    MQTT_PORTA
  );

  mqtt.setBufferSize(2048);

  conectarMQTT();

  // Primeira localizacao

  if (
    obterLocalizacaoWiFi()
  ) {

    enviarLocalizacao();

  }

  enviarBateria();

  enviarTelemetria();

  ultimoEnvio =
    millis();
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  // Reconectar WiFi

  if (
    WiFi.status() !=
    WL_CONNECTED
  ) {

    conectarWiFi();

  }

  // Reconectar MQTT

  if (
    !mqtt.connected()
  ) {

    conectarMQTT();

  }

  mqtt.loop();

  // Enviar a cada 60 segundos

  if (
    millis() - ultimoEnvio
    >= INTERVALO_ENVIO
  ) {

    ultimoEnvio =
      millis();

    if (
      obterLocalizacaoWiFi()
    ) {

      enviarLocalizacao();

    }

    enviarBateria();

    enviarTelemetria();
  }

  delay(10);
}
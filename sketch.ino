// =======================
// MindSkill IoT – Estação de Bem-Estar e Produtividade
// ESP32 + DHT22 + Potenciometro + Botão + LEDs + LCD I2C + MQTT
// =======================

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

// -----------------------
// CONFIG WIFI + MQTT
// -----------------------
const char* ssid        = "Wokwi-GUEST";   // Padrão do Wokwi
const char* password    = "";              // Sem senha
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

// Tópicos MQTT
const char* topicTemperatura = "/mindskill/temperatura";
const char* topicUmidade     = "/mindskill/umidade";
const char* topicStress      = "/mindskill/stress";
const char* topicEstado      = "/mindskill/estado";
const char* topicJson        = "/mindskill/payload";
// tópico de comando:
const char* topicCommandPause = "/mindskill/commands/pause";

// -----------------------
// PINAGEM
// -----------------------
#define DHTPIN   15
#define DHTTYPE  DHT22

const int STRESS_PIN      = 35;   // ADC (potenciômetro)
const int LED_VERDE_PIN   = 5;
const int LED_AMARELO_PIN = 4;
const int LED_VERMELHO_PIN = 2;
const int BOTAO_PIN       = 14;   // push button (ligado em GND, INPUT_PULLUP)

// LCD I2C (endereço comum: 0x27)
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

// -----------------------
// VARIÁVEIS DE ESTADO
// -----------------------
bool lastButtonState = HIGH;  // por causa do INPUT_PULLUP
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

unsigned long lastPublishMillis = 0;
const unsigned long publishInterval = 5000; // 5 segundos

unsigned long lastPauseMillis = 0;              // ultima vez que entrou em pausa
const unsigned long maxSemPausa = 20UL * 60UL * 1000UL; // 20 min (exemplo)

// flag de comando remoto:
bool pausaRemotaSolicitada = false;

// -----------------------
// FUNÇÃO: callback MQTT (recebe comandos)
// -----------------------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String mensagem;
  for (unsigned int i = 0; i < length; i++) {
    mensagem += (char)payload[i];
  }

  Serial.print("MQTT recebido em ");
  Serial.print(topic);
  Serial.print(" => ");
  Serial.println(mensagem);

  // Se chegou no tópico de pausa:
  if (String(topic) == String(topicCommandPause)) {
    mensagem.toUpperCase();
    if (mensagem == "1" || mensagem == "PAUSE" || mensagem == "PAUSA") {
      pausaRemotaSolicitada = true;
      Serial.println(">> Comando remoto de PAUSA recebido!");
    }
  }
}

// -----------------------
// FUNÇÕES DE REDE
// -----------------------
void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.println("Conectando ao WiFi...");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT em ");
    Serial.print(mqtt_server);
    Serial.println(" ...");

    String clientId = "MindSkillESP32-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("MQTT conectado!");

      // assina o tópico de comandos
      client.subscribe(topicCommandPause);
      Serial.print("Assinado: ");
      Serial.println(topicCommandPause);

    } else {
      Serial.print("Falhou, rc = ");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

// -----------------------
// FUNÇÃO: Lê sensores
// -----------------------
void lerSensores(float &temperatura, float &umidade, int &stressPercent) {
  // DHT
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // °C

  if (isnan(h) || isnan(t)) {
    Serial.println("Falha ao ler DHT22!");
  } else {
    umidade = h;
    temperatura = t;
  }

  // STRESS (potenciômetro)
  int stressRaw = analogRead(STRESS_PIN); // 0–4095
  stressPercent = map(stressRaw, 0, 4095, 0, 100);
}

// -----------------------
// FUNÇÃO: Decide estado (normal/moderado/crítico)
// -----------------------
String calcularEstado(float temperatura, float umidade, int stressPercent) {
  unsigned long agora = millis();
  unsigned long tempoSemPausa = agora - lastPauseMillis;

  bool alertaStress      = stressPercent >= 60;
  bool alertaStressAlto  = stressPercent >= 80;
  bool alertaTemp        = (temperatura > 28 || temperatura < 10);
  bool alertaTempAlta    = temperatura > 30;
  bool semPausaMtTempo   = tempoSemPausa > maxSemPausa; // muito tempo sem pausa

  // Estado crítico:
  if (alertaStressAlto || (alertaTempAlta && alertaStress) || (alertaStress && semPausaMtTempo)) {
    return "CRITICO";
  }

  // Estado moderado:
  if (alertaStress || alertaTemp) {
    return "MODERADO";
  }

  // Estado normal:
  return "NORMAL";
}

// -----------------------
// FUNÇÃO: Atualiza LEDs conforme estado
// -----------------------
void atualizarLeds(String estado) {
  if (estado == "NORMAL") {
    digitalWrite(LED_VERDE_PIN, HIGH);
    digitalWrite(LED_AMARELO_PIN, LOW);
    digitalWrite(LED_VERMELHO_PIN, LOW);
  } else if (estado == "MODERADO") {
    digitalWrite(LED_VERDE_PIN, LOW);
    digitalWrite(LED_AMARELO_PIN, HIGH);
    digitalWrite(LED_VERMELHO_PIN, LOW);
  } else if (estado == "CRITICO") {
    digitalWrite(LED_VERDE_PIN, LOW);
    digitalWrite(LED_AMARELO_PIN, LOW);
    digitalWrite(LED_VERMELHO_PIN, HIGH);
  }
}

// -----------------------
// FUNÇÃO: Atualiza LCD
// -----------------------
void atualizarLCD(String estado, float temperatura, int stressPercent) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MindSkill IoT");

  lcd.setCursor(0, 1);

  if (estado == "NORMAL") {
    lcd.print("OK T:");
    lcd.print(temperatura, 1);
    lcd.print("C S:");
    lcd.print(stressPercent);
  } else if (estado == "MODERADO") {
    lcd.print("Ajuste T:");
    lcd.print(temperatura, 0);
    lcd.print(" S:");
    lcd.print(stressPercent);
  } else if (estado == "CRITICO") {
    // Mensagem de alerta incentivando pausa
    lcd.print("CRITICO: pausa");
  }
}

// -----------------------
// FUNÇÃO: Verifica se o botão foi pressionado
// -----------------------
bool botaoFoiPressionado() {
  int leitura = digitalRead(BOTAO_PIN);
  bool pressionado = (leitura == LOW && lastButtonState == HIGH); // HIGH -> LOW
  lastButtonState = leitura;
  return pressionado;
}

// -----------------------
// FUNÇÃO: Publica no MQTT
// -----------------------
void publicarMQTT(float temperatura, float umidade, int stressPercent, String estado) {
  // Publica valores simples
  client.publish(topicTemperatura, String(temperatura, 1).c_str(), true);
  client.publish(topicUmidade,     String(umidade, 1).c_str(), true);
  client.publish(topicStress,      String(stressPercent).c_str(), true);
  client.publish(topicEstado,      estado.c_str(), true);

  // Monta JSON
  String payload = "{";
  payload += "\"temperatura\":" + String(temperatura, 1) + ",";
  payload += "\"umidade\":"     + String(umidade, 1) + ",";
  payload += "\"stress\":"      + String(stressPercent) + ",";
  payload += "\"estado\":\""    + estado + "\"";
  payload += "}";

  client.publish(topicJson, payload.c_str(), true);

  Serial.println("MQTT enviado: " + payload);
}

// =======================
// SETUP
// =======================
void setup() {
  Serial.begin(115200);
  randomSeed(micros());

  // Inicia sensores
  dht.begin();

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("MindSkill IoT");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");

  // Pinos
  pinMode(LED_VERDE_PIN, OUTPUT);
  pinMode(LED_AMARELO_PIN, OUTPUT);
  pinMode(LED_VERMELHO_PIN, OUTPUT);
  pinMode(BOTAO_PIN, INPUT_PULLUP);
  lastButtonState = digitalRead(BOTAO_PIN);

  // Estado inicial
  digitalWrite(LED_VERDE_PIN, LOW);
  digitalWrite(LED_AMARELO_PIN, LOW);
  digitalWrite(LED_VERMELHO_PIN, LOW);

  // Rede
  setupWiFi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);   // <<< importante!

  lastPauseMillis = millis(); // considera que começou "descansado"
}

// =======================
// LOOP
// =======================
void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop(); // trata comunicação MQTT

  float temperatura = 0.0;
  float umidade = 0.0;
  int stressPercent = 0;

  // Lê sensores
  lerSensores(temperatura, umidade, stressPercent);

  // Calcula estado atual
  String estado = calcularEstado(temperatura, umidade, stressPercent);

  // Verifica se o botão foi pressionado
  bool pressionado = botaoFoiPressionado();

  // Se estamos em estado CRÍTICO e o botão foi pressionado
  // OU se chegou comando remoto de pausa:
  if ( (estado == "CRITICO" && pressionado) || pausaRemotaSolicitada ) {

    bool pausaRemota = pausaRemotaSolicitada;
    pausaRemotaSolicitada = false; // consome o comando

    Serial.println("Pausa MindSkill ativada ("
                   + String(pausaRemota ? "remota" : "botao") + ")");

    // Mensagem de pausa no LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dando uma pausa");
    lcd.setCursor(0, 1);
    if (pausaRemota) {
      lcd.print("Pausa via app :)");
    } else {
      lcd.print("Melhorando amb.");
    }
    delay(2000); // 2 segundos mostrando a pausa

    // Simula que a pessoa descansou:
    temperatura   = 25.0;  // temp confortável
    stressPercent = 30;    // stress moderado/baixo
    lastPauseMillis = millis();

    // Força estado para NORMAL após a pausa
    estado = "NORMAL";
  }

  // Atualiza LEDs e LCD com o estado (já considerando reset)
  atualizarLeds(estado);
  atualizarLCD(estado, temperatura, stressPercent);

  // Publica MQTT a cada intervalo
  unsigned long agora = millis();
  if (agora - lastPublishMillis > publishInterval) {
    lastPublishMillis = agora;
    publicarMQTT(temperatura, umidade, stressPercent, estado);
  }

  delay(500); // loop mais suave
}

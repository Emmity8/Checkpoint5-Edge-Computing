#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>

#define DHTPIN 15 
#define DHTTYPE DHT11
const int ldrPin = 34;

DHT dht(DHTPIN, DHTTYPE);

const char* default_SSID = "FIAP-IOT";
const char* default_PASSWORD = "F!@p25.IOT";
const char* default_BROKER_MQTT = "Ip do servidor";
const int default_BROKER_PORT = 1883;
const int default_D4 = 2;
const char* default_TOPICO_SUBSCRIBE = "/TEF/device007/cmd";
const char* default_TOPICO_PUBLISH = "/TEF/device007/attrs";
const char* default_ID_MQTT = "fiware_007";
const char* topicPrefix = "device007";

char* SSID = const_cast<char*>(default_SSID);
char* PASSWORD = const_cast<char*>(default_PASSWORD);
char* BROKER_MQTT = const_cast<char*>(default_BROKER_MQTT);
int BROKER_PORT = default_BROKER_PORT;
char* TOPICO_SUBSCRIBE = const_cast<char*>(default_TOPICO_SUBSCRIBE);
char* TOPICO_PUBLISH = const_cast<char*>(default_TOPICO_PUBLISH);
char* ID_MQTT = const_cast<char*>(default_ID_MQTT);
int D4 = default_D4;

WiFiClient espClient;
PubSubClient MQTT(espClient);
char EstadoSaida = '0';

unsigned long lastSensorRead = 0;
const long sensorInterval = 10000;

void setup() {
    InitOutput();
    initSerial();
    dht.begin();
    initWiFi();
    initMQTT();
    delay(1000); 
    MQTT.publish(TOPICO_PUBLISH, "s|off");
}

void loop() {
    VerificaConexoesWiFIEMQTT();
    MQTT.loop();

    unsigned long now = millis();
    if (now - lastSensorRead > sensorInterval) {
        lastSensorRead = now;
        publishSensorData();
    }
}

void initSerial() {
    Serial.begin(115200);
}

void initWiFi() {
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
    reconectWiFi();
}

void initMQTT() {
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);
    MQTT.setCallback(mqtt_callback);

void InitOutput() {
    pinMode(D4, OUTPUT);
    digitalWrite(D4, HIGH);
    boolean toggle = false;

    for (int i = 0; i <= 10; i++) {
        toggle = !toggle;
        digitalWrite(D4, toggle);
        delay(200);
    }
    digitalWrite(D4, LOW);
}

void reconectWiFi() {
    if (WiFi.status() == WL_CONNECTED)
        return;
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println(" IP obtido: ");
    Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
    while (!MQTT.connected()) {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE);
        } else {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Haverá nova tentativa de conexão em 2s");
            delay(2000);
        }
    }
}

void VerificaConexoesWiFIEMQTT() {
    if (!MQTT.connected())
        reconnectMQTT();
    reconectWiFi();
}


void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (int i = 0; i < length; i++) {
        char c = (char)payload[i];
        msg += c;
    }
    Serial.print("- Mensagem recebida: ");
    Serial.println(msg);

    String onTopic = String(topicPrefix) + "@on|";
    String offTopic = String(topicPrefix) + "@off|";

    String publishPayload = "";

    if (msg.equals(onTopic)) {
        digitalWrite(D4, HIGH);
        EstadoSaida = '1';
        publishPayload = "s|on";
        Serial.println("- LED Ligado");
    }

    if (msg.equals(offTopic)) {
        digitalWrite(D4, LOW);
        EstadoSaida = '0';
        publishPayload = "s|off";
        Serial.println("- LED Desligado");
    }

    if (publishPayload.length() > 0) {
        MQTT.publish(TOPICO_PUBLISH, publishPayload.c_str());
        Serial.println("- Estado do LED enviado ao broker!");
    }
}

void publishSensorData() {
    int sensorValue = analogRead(ldrPin);
    int luminosity = map(sensorValue, 0, 4095, 0, 100); 

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
        Serial.println("Falha ao ler o sensor DHT11!");
        return;
    }

    String payload = "l|" + String(luminosity) + "|t|" + String(t, 1) + "|h|" + String(h, 1);

    Serial.print("Publicando dados dos sensores: ");
    Serial.println(payload);
    MQTT.publish(TOPICO_PUBLISH, payload.c_str());
}
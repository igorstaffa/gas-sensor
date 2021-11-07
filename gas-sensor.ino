#define LED_G_PORT    14
#define LED_Y_PORT    12
#define LED_R_PORT    13
#define GAS_PORT      A0
#define WIFI_SSID    "TIRONI 2.4"
#define WIFI_PASS    "tironi0303"

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const long    interval          = 4000;
unsigned long previousMillis    = 0;

// MQTT
const char*   MQTT_BROKER       = "broker.hivemq.com";
int           MQTT_BROKER_PORT  = 1883;
const char*   MQTT_CLIENT_ID    = "gas_sensor_matd02";
const char    MQTT_GAS_TOPIC[]  = "MATD02_GasMonitor";

WiFiClient    espClient;
PubSubClient  MQTT(espClient);

void initSerial();
void initPins();
void initWiFi();
void initMQTT();

void reconectWiFi();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void updateLeds(int gasReadValue);
void reconnect();

void setup() {
    initSerial();
    initPins();
    initWiFi();
    initMQTT();
}

void initSerial() {
    Serial.begin(9600);
}

void initPins() {
    pinMode(LED_R_PORT, OUTPUT);
    pinMode(LED_Y_PORT, OUTPUT);
    pinMode(LED_G_PORT, OUTPUT);

    digitalWrite(LED_Y_PORT,  HIGH);
}

void initWiFi() {
    delay(2000);

    Serial.println();
    Serial.print("Connecting to WiFi: ");
    Serial.println(WIFI_SSID);

    reconectWiFi();
}

void initMQTT() {
    MQTT.setServer(MQTT_BROKER, MQTT_BROKER_PORT);
    MQTT.setCallback(mqtt_callback);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String msg;

    for(int i = 0; i < length; i++) {
       char c = (char)payload[i];
       msg += c;
    }

    Serial.print("Message Received: ");
    Serial.println(msg);
}

void reconnectMQTT() {
    if (MQTT.connected()) {
        return;
    }

    Serial.print("Connecting to MQTT Broker: ");
    Serial.println(MQTT_BROKER);

    while (!MQTT.connected()) {
        if (MQTT.connect(MQTT_CLIENT_ID)) {
            Serial.println("Connected to MQTT Broker!");
            MQTT.subscribe(MQTT_GAS_TOPIC);

            continue;
        }

        Serial.println("Failed to connect to MQTT Broker");
        delay(2000);
    }
}

void reconectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        return;
    }

    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("Connected to WiFi");
}

void reconnect()
{
    reconnectMQTT();
    reconectWiFi();
}

void updateLeds(int gasReadValue) {
    digitalWrite(LED_G_PORT,  HIGH);
    digitalWrite(LED_Y_PORT,  gasReadValue >= 60 ? HIGH : LOW);
    digitalWrite(LED_R_PORT , gasReadValue >= 80 ? HIGH : LOW);
}

void updateGasRead(int gasReadValue) {
    updateLeds(gasReadValue);

    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis < interval) {
        return;
    }

    previousMillis = currentMillis;

    String gasReadString = String(gasReadValue);
    char data[80];

    gasReadString.toCharArray(data, (gasReadString.length() + 1));
    MQTT.publish(MQTT_GAS_TOPIC, data);
}

void loop() {
    reconnect();

    MQTT.loop();

    int gasReadRawValue = analogRead(GAS_PORT);
    int gasReadValue = map(gasReadRawValue, 300, 750, 0, 100);

    updateGasRead(gasReadValue);

    delay(250);
}

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* mqttServer = "broker.hivemq.com";
int mqttPort = 1883;
const char* temperatureTopic = "21127577/temperature";
const char* gpsTopic = "21127577/gps";
const char* ringBellTopic = "21127577/ringBell";

#define DHT_PIN 15      // Chân D15 của ESP32
#define LED_PIN 13      // Chân D13 của ESP32 cho đèn LED
#define BUZZER_PIN 12   // Chân D12 của ESP32 cho chuông

DHTesp dht;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void wifiConnect() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
}

void mqttConnect() {
  while (!mqttClient.connected()) {
    Serial.println("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("Connected");
      mqttClient.subscribe(ringBellTopic);
    } else {
      Serial.println("Try again in 5 seconds");
      delay(5000);
    }
  }
}
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");

  String message = ""; 
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message[length] = '\0'; 

  Serial.println(message);

  if (String(topic) == ringBellTopic) {
    if (message == "true") {
      ringBellAndFlashLED(true);
    } else if (message == "false") { 
      ringBellAndFlashLED(false);
    }
  }
}


void publishTemperature(float temperature) {
  mqttClient.publish(temperatureTopic, String(temperature).c_str());
}

void publishGPS(float latitude, float longitude) {
  char gpsData[100];
  snprintf(gpsData, sizeof(gpsData), "{\"lat\":%.6f,\"lon\":%.6f}", latitude, longitude);
  mqttClient.publish(gpsTopic, gpsData);
}

void setup() {
  Serial.begin(9600);
  Serial.print("Connecting to WiFi");

  wifiConnect();
  mqttClient.setCallback(mqttCallback);
  mqttClient.setServer(mqttServer, mqttPort);
  dht.setup(DHT_PIN, DHTesp::DHT22);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.println("Pet Monitoring System is ready!");
}


void loop() {
  if (!mqttClient.connected()) {
    mqttConnect();
  }
  mqttClient.loop();

  if (!mqttClient.connected()) {
    mqttConnect();
  }
  mqttClient.loop();

  delay(5000);

  // Đo nhiệt độ
  float temperature = dht.getTemperature();

  if (!isnan(temperature)) {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println("°C");

    publishTemperature(temperature);
  } else {
    Serial.println("Failed to read from DHT sensor!");
  }
  

  // Giả lập dữ liệu GPS
  static float latitude = 10.749959529955023;
  static float longitude = 106.69620475472809;

  Serial.print("Latitude: ");
  Serial.println(latitude, 6);
  Serial.print("Longitude: ");
  Serial.println(longitude, 6);

  publishGPS(latitude, longitude);

}
void ringBellAndFlashLED(bool turnOn) {
  if (turnOn) {

    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, 1000); 
    digitalWrite(LED_PIN, HIGH); 
  } else {
    // Stop playing the tone
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
    digitalWrite(LED_PIN, LOW); 
  }
}

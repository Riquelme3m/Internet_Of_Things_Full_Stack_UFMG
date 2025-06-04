#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <time.h>

// --- Pin Definitions ---
const int alarmLED = 14;
const int servoPin = 27;
const int tempHumiditySensor = 26;

// --- WiFi & MQTT Configuration ---
const char* ssid = "Iphone de Thalys";
const char* pswrd = "12345678";
const char* mqtt_server = "200.238.239.146";

// --- Sensor and Actuator Metadata ---
const char* machine_id = "ESP32_01";
const char* sensorTempId = "DTH11_temp";
const char* humiditySensorId = "DTH11_humidity";
const char* alarmLedId = "alarmLED";
const char* servoMotorId = "servo_motor_01";

const char* sensorTempDataType = "Temperature °C";
const char* sensorHumidityDataType = "Humidity";
const char *alarmLedDataType = "ON/OFF";
const char* servoMotorDataType = "ON/OFF";

// --- Global Objects ---
WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(tempHumiditySensor, DHT11);
Servo myServo;

// --- Timing Control ---
unsigned long lastSensorRead = 0;
unsigned long lastStatePublish = 0;
const unsigned long sensorInterval = 5000;
const unsigned long stateInterval = 500;

// --- State ---
float temperatureThreshold = 30.0; // Default threshold

// --- Helper: Get ISO 8601 Timestamp ---
String getISO8601Timestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "1970-01-01T00:00:00Z";
  }
  char buf[25];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(buf);
}

// --- MQTT: Publish Initial Sensor Info ---
void publishInitialSensorInfo() {
  StaticJsonDocument<512> doc;
  doc["machine_id"] = machine_id;

  JsonArray sensors = doc.createNestedArray("sensors");
  JsonArray actuators = doc.createNestedArray("actuators");

  JsonObject sensor1 = sensors.createNestedObject();
  sensor1["sensor_id"] = sensorTempId;
  sensor1["data_type"] = sensorTempDataType;
  sensor1["data_interval"] = sensorInterval;

  JsonObject sensor2 = sensors.createNestedObject();
  sensor2["sensor_id"] = humiditySensorId;
  sensor2["data_type"] = sensorHumidityDataType;
  sensor2["data_interval"] = sensorInterval;

  JsonObject sensor3 = sensors.createNestedObject();
  sensor3["sensor_id"] = alarmLedId;
  sensor3["data_type"] = alarmLedDataType;
  sensor3["data_interval"] = "No specified data interval";

  JsonObject actuator = actuators.createNestedObject();
  actuator["actuator_id"] = servoMotorId;
  actuator["data_type"] = servoMotorDataType;
  actuator["data_interval"] = "No specified data interval";

  char buffer[512];
  size_t n = serializeJson(doc, buffer);
  client.publish("/sensor_monitors", buffer, n);
  Serial.println("Initial sensor info published:");
  Serial.println(buffer);
}

// --- MQTT: Publish Sensor Data ---
void publishTemperatureSensorData(float temp) {
  StaticJsonDocument<128> doc;
  doc["timestamp"] = getISO8601Timestamp();
  doc["value"] = temp;
  char buffer[128];
  size_t n = serializeJson(doc, buffer);
  client.publish("/sensors/ESP32_01/DTH11_temp", buffer, n);
}

void publishHumiditySensorData(float humidity) {
  StaticJsonDocument<128> doc;
  doc["timestamp"] = getISO8601Timestamp();
  doc["value"] = humidity;
  char buffer[128];
  size_t n = serializeJson(doc, buffer);
  client.publish("/sensors/ESP32_01/DTH11_humidity", buffer, n);
}

void publishAlarmState() {
  StaticJsonDocument<128> doc;
  doc["timestamp"] = getISO8601Timestamp();
  doc["value"] = digitalRead(alarmLED) == HIGH ? "ON" : "OFF";
  char buffer[128];
  size_t n = serializeJson(doc, buffer);
  client.publish("/states/ESP32_01/alarmLED", buffer, n);
}

void publishServoMotorState() {
  StaticJsonDocument<128> doc;
  doc["timestamp"] = getISO8601Timestamp();
  doc["value"] = myServo.read() > 90 ? "ON" : "OFF";
  char buffer[128];
  size_t n = serializeJson(doc, buffer);
  client.publish("/states/ESP32_01/servo_motor_01", buffer, n);
}

// --- WiFi Setup ---
void setup_wifi() {
  delay(10);
  Serial.print("Connecting to WiFi network: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pswrd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    
    Serial.print(" Status: ");
    Serial.println(WiFi.status());
  }
  Serial.println("\nWiFi connected successfully");
}

// --- MQTT: Callback to Receive Temperature Threshold ---

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Received on topic ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(message);

  // Use strcmp for topic comparison
  if (strcmp(topic, "esp/temp_threshold") == 0) {
    temperatureThreshold = message.toFloat();
    Serial.print("Updated temperature threshold to: ");
    Serial.println(temperatureThreshold);
  } 
  else if (strcmp(topic, "esp/servo_control") == 0) {
    if (message.equalsIgnoreCase("ON")) {
      myServo.write(180);
      Serial.println("Servo turned ON via MQTT");
    } 
    else if (message.equalsIgnoreCase("OFF")) {
      myServo.write(0);
      Serial.println("Servo turned OFF via MQTT");
    }
  }
}
// ...existing code...


// --- MQTT: Reconnect if Needed ---
// Add a global flag at the top of your file:
bool initialInfoPublished = false;

// --- MQTT: Reconnect if Needed ---
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client_01")) {
      Serial.println("connected");
      client.subscribe("esp/temp_threshold");
      client.subscribe("esp/servo_control");
      Serial.println("SUbscribed");
      if (!initialInfoPublished) {
        publishInitialSensorInfo();
        initialInfoPublished = true;
      }
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(". Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  configTime(0, 0, "pool.ntp.org");

  dht.begin();
  pinMode(alarmLED, OUTPUT);
  myServo.attach(servoPin);

  client.setServer(mqtt_server, 1883);
  client.setKeepAlive(60);
  client.setCallback(mqttCallback);
  
}

// --- Main Loop ---
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long now = millis();

  // Every 5 seconds: read sensors
  if (now - lastSensorRead >= sensorInterval) {
    lastSensorRead = now;
    float temp = dht.readTemperature();
    float humidity = dht.readHumidity();
    Serial.print("Temperature: ");
    Serial.println(temp);
    Serial.print("Humidity: ");
    Serial.println(humidity);
    

    if (!isnan(temp) && !isnan(humidity)) {
      
      publishTemperatureSensorData(temp);
      publishHumiditySensorData(humidity);
      Serial.println(temperatureThreshold);
      if (temp > temperatureThreshold) {
        digitalWrite(alarmLED, HIGH);
        
      } else {
        digitalWrite(alarmLED, LOW);
        
      }
    }
  }

  // Every 1 second: publish states
  if (now - lastStatePublish >= stateInterval) {
    lastStatePublish = now;
    publishAlarmState();
    publishServoMotorState();
  }
}

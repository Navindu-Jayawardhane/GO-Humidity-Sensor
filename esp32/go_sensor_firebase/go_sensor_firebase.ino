#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

#define DAC_PIN 25
#define ADC_PIN 34

const int DAC_VALUE = 78;

const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const char* FIREBASE_API_KEY = "AIzaSyB7sQ-PZnhfBufhDovdoT0yDe1mzpukWHc";
const char* FIREBASE_DATABASE_URL =
    "https://esp32-smart-environment-9624f-default-rtdb.europe-west1.firebasedatabase.app";

const char* FIREBASE_EMAIL = "go-esp32@yourproject.com";
const char* FIREBASE_PASSWORD = "YOUR_FIREBASE_DEVICE_PASSWORD";

const char* DEVICE_ID = "GO_SENSOR_01";

const unsigned long UPLOAD_INTERVAL = 1000;

String firebaseIdToken = "";
unsigned long tokenExpiresAt = 0;

float readSensorVoltage() {
  // Original voltage-reading method from the working local dashboard.
  int adcValue = analogRead(ADC_PIN);
  float voltage = adcValue * (3.3 / 4095.0);
  return voltage;
}

bool firebaseLogin() {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;

  String url =
      "https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=";
  url += FIREBASE_API_KEY;

  if (!https.begin(client, url)) return false;

  https.addHeader("Content-Type", "application/json");

  StaticJsonDocument<512> request;
  request["email"] = FIREBASE_EMAIL;
  request["password"] = FIREBASE_PASSWORD;
  request["returnSecureToken"] = true;

  String body;
  serializeJson(request, body);

  int httpCode = https.POST(body);

  if (httpCode != 200) {
    Serial.print("Firebase login failed: ");
    Serial.println(httpCode);
    Serial.println(https.getString());
    https.end();
    return false;
  }

  String response = https.getString();

  StaticJsonDocument<2048> responseDoc;
  DeserializationError error = deserializeJson(responseDoc, response);

  if (error) {
    Serial.println("Firebase JSON parsing failed.");
    https.end();
    return false;
  }

  firebaseIdToken = responseDoc["idToken"].as<String>();

  unsigned long expiresIn =
      responseDoc["expiresIn"].as<unsigned long>();

  tokenExpiresAt = millis() + expiresIn * 1000UL;

  Serial.println("Firebase authentication successful.");

  https.end();
  return true;
}

bool uploadCurrentReading(float voltage, unsigned long timestamp) {
  if (firebaseIdToken.length() == 0) return false;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;

  String url = String(FIREBASE_DATABASE_URL);
  url += "/devices/";
  url += DEVICE_ID;
  url += ".json?auth=";
  url += firebaseIdToken;

  if (!https.begin(client, url)) return false;

  https.addHeader("Content-Type", "application/json");

  StaticJsonDocument<256> data;
  data["voltage"] = voltage;
  data["timestamp"] = timestamp;
  data["status"] = "online";

  String body;
  serializeJson(data, body);

  int httpCode = https.PUT(body);
  https.end();

  return httpCode == 200;
}

bool uploadHistoricalReading(float voltage, unsigned long timestamp) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;

  String url = String(FIREBASE_DATABASE_URL);
  url += "/readings/";
  url += DEVICE_ID;
  url += ".json?auth=";
  url += firebaseIdToken;

  if (!https.begin(client, url)) return false;

  https.addHeader("Content-Type", "application/json");

  StaticJsonDocument<256> data;
  data["voltage"] = voltage;
  data["timestamp"] = timestamp;

  String body;
  serializeJson(data, body);

  int httpCode = https.POST(body);
  https.end();

  return httpCode == 200;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("================================");
  Serial.println("GO SENSOR - FIREBASE IoT SYSTEM");
  Serial.println("================================");

  pinMode(ADC_PIN, INPUT);
  dacWrite(DAC_PIN, DAC_VALUE);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Wi-Fi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  firebaseLogin();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi disconnected. Reconnecting...");
    WiFi.reconnect();
    delay(1000);
    return;
  }

  if (firebaseIdToken.length() == 0 ||
      millis() > tokenExpiresAt - 60000UL) {
    Serial.println("Refreshing Firebase authentication...");
    firebaseLogin();
  }

  static unsigned long lastUpload = 0;

  if (millis() - lastUpload >= UPLOAD_INTERVAL) {
    lastUpload = millis();

    float voltage = readSensorVoltage();
    unsigned long timestamp = millis() / 1000;

    Serial.print("Voltage: ");
    Serial.print(voltage, 4);
    Serial.println(" V");

    bool currentOK = uploadCurrentReading(voltage, timestamp);
    bool historyOK = uploadHistoricalReading(voltage, timestamp);

    if (currentOK && historyOK) {
      Serial.println("Firebase upload OK.");
    } else {
      Serial.println("Firebase upload problem.");
    }
  }
}

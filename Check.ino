#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <MPU6050.h>
#include <ArduinoJson.h>

// ---------------- WiFi ----------------
const char* ssid = "Shidlyali_F_Floor_2.4GHz";
const char* password = "Shidlyali*2025#";

// ---------------- Google Script URL ----------------
const String SCRIPT_BASE_URL = "https://script.google.com/macros/s/YOUR_SCRIPT_ID/exec";

// ---------------- Telegram ----------------
const String BOTtoken = "8584168475:AAG8lYazrM8mrz5qn5zw6HDJpW_7TeMYEzU";
const String CHAT_ID  = "1640526464";

// ---------------- MPU6050 ----------------
MPU6050 mpu;
bool alertAlreadySent = false;


// calibration offsets
long ax_off = 0, ay_off = 0, az_off = 0;
long gx_off = 0, gy_off = 0, gz_off = 0;

bool alertSent = false;

// previous values for SYNC
float prevAx = 0, prevAy = 0, prevAz = 0;


// ----------- CALIBRATION FUNCTION -------------
void calibrateMPU() {
  Serial.println("Calibrating MPU6050... keep device still.");

  const int samples = 500;
  long ax_sum = 0, ay_sum = 0, az_sum = 0;
  long gx_sum = 0, gy_sum = 0, gz_sum = 0;

  for (int i = 0; i < samples; i++) {
    int16_t axr, ayr, azr, gxr, gyr, gzr;
    mpu.getMotion6(&axr, &ayr, &azr, &gxr, &gyr, &gzr);

    ax_sum += axr;
    ay_sum += ayr;
    az_sum += (azr - 16384);  // remove gravity (1g)
    gx_sum += gxr;
    gy_sum += gyr;
    gz_sum += gzr;

    delay(3);
  }

  ax_off = ax_sum / samples;
  ay_off = ay_sum / samples;
  az_off = az_sum / samples;
  gx_off = gx_sum / samples;
  gy_off = gy_sum / samples;
  gz_off = gz_sum / samples;

  Serial.println("Calibration done.");
}


// ----------- READ MPU FUNCTION -------------
void readMPU(float &ax, float &ay, float &az, float &gx, float &gy, float &gz) {
  int16_t axr, ayr, azr, gxr, gyr, gzr;
  mpu.getMotion6(&axr, &ayr, &azr, &gxr, &gyr, &gzr);

  ax = (axr - ax_off) / 16384.0;
  ay = (ayr - ay_off) / 16384.0;
  az = (azr - az_off) / 16384.0;

  gx = (gxr - gx_off) / 131.0;
  gy = (gyr - gy_off) / 131.0;
  gz = (gzr - gz_off) / 131.0;
}


// ----------- TELEGRAM FUNCTION -------------
void sendTelegramAlert(String message) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  https.begin(client, "https://api.telegram.org/bot" + BOTtoken + "/sendMessage");
  https.addHeader("Content-Type", "application/json");

  String payload = "{\"chat_id\":\"" + CHAT_ID + "\",\"text\":\"" + message + "\"}";
  https.POST(payload);
  https.end();

  Serial.println("Telegram Alert Sent!");
}


// ----------- ACC THRESHOLD CHECK -------------
bool fetchACCThresholdAndCheck(float accA, float accB, float accC, float accD, float accE) {

  HTTPClient http;
  http.begin(SCRIPT_BASE_URL + "?type=ACC");

  int code = http.GET();
  if (code != 200) {
    Serial.println("Error loading ACC dataset");
    return false;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(6000);
  if (deserializeJson(doc, payload)) {
    Serial.println("JSON parse error!");
    return false;
  }

  JsonArray arr = doc.as<JsonArray>();
  if (arr.size() == 0) {
    Serial.println("Dataset empty.");
    return false;
  }

  // ------------ TEST MODE: ONLY FIRST ROW ------------
  JsonObject row = arr[0];

  float thrA = row["accA"];
  float thrB = row["accB"];
  float thrC = row["accC"];
  float thrD = row["accD"];
  float thrE = row["accE"];

  // PRINT THE THRESHOLD YOU ARE USING (GOOD FOR CHECKING)
  Serial.print("Testing Against → A:");
  Serial.print(thrA);
  Serial.print(" B:");
  Serial.print(thrB);
  Serial.print(" C:");
  Serial.print(thrC);
  Serial.print(" D:");
  Serial.print(thrD);
  Serial.print(" E:");
  Serial.println(thrE);

  // ------------ TRIGGER CONDIT



// ----------- SYNC THRESHOLD CHECK -------------
bool fetchSYNCThresholdAndCheck(float X, float Y, float Z) {

  HTTPClient http;
  http.begin(SCRIPT_BASE_URL + "?type=SYNC");

  int code = http.GET();
  if (code != 200) { http.end(); return false; }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(20000);
  deserializeJson(doc, payload);

  for (JsonObject row : doc.as<JsonArray>()) {
    float tX = row["SYN_X"];
    float tY = row["SYN_Y"];
    float tZ = row["SYN_Z"];

    if (abs(X) > tX || abs(Y) > tY || abs(Z) > tZ) {
      Serial.println("🔔 SYNC Trigger:");
      Serial.println(String("SYN_X: ") + tX + " SYN_Y: " + tY + " SYN_Z: " + tZ);
      return true;
    }
  }
  return false;
}


// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  // MPU6050 INIT
  Wire.begin();
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed!");
    while (1);
  }

  calibrateMPU();
  sendTelegramAlert("🚀 ESP32 Started & Calibrated!");
}


// ================= LOOP =================
void loop() {

  float ax, ay, az, gx, gy, gz;
  readMPU(ax, ay, az, gx, gy, gz);

  // Print live MPU values
  Serial.print("ACC → ");
  Serial.print(ax); Serial.print(", ");
  Serial.print(ay); Serial.print(", ");
  Serial.print(az);
  Serial.print(" | GYRO → ");
  Serial.print(gx); Serial.print(", ");
  Serial.print(gy); Serial.print(", ");
  Serial.println(gz);

  // Map values (your ACC features)
  float accA = ax;
  float accB = ay;
  float accC = az;
  float accD = gx;
  float accE = gy;

  bool testTrigger = fetchACCThresholdAndCheck(accA, accB, accC, accD, accE);

  if (testTrigger && !alertAlreadySent) {
    sendTelegramAlert("🔥 TEST FALL DETECTED! — Threshold exceeded");
    alertAlreadySent = true;
  }

  delay(300);
}

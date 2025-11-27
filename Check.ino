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

// calibration offsets
long ax_off = 0, ay_off = 0, az_off = 0;
long gx_off = 0, gy_off = 0, gz_off = 0;

bool alertSent = false;

// sync previous values
float prevAx = 0, prevAy = 0, prevAz = 0;


// ============================================================
//                  CALIBRATE MPU6050
// ============================================================
void calibrateMPU() {
  Serial.println("Calibrating MPU6050... Keep device still.");

  const int samples = 500;
  long ax_sum = 0, ay_sum = 0, az_sum = 0;
  long gx_sum = 0, gy_sum = 0, gz_sum = 0;

  for (int i = 0; i < samples; i++) {
    int16_t axr, ayr, azr, gxr, gyr, gzr;
    mpu.getMotion6(&axr, &ayr, &azr, &gxr, &gyr, &gzr);

    ax_sum += axr;
    ay_sum += ayr;
    az_sum += (azr - 16384);  
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

  Serial.println("Calibration Complete.");
}


// ============================================================
//                  READ MPU VALUES
// ============================================================
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


// ============================================================
//                SEND TELEGRAM MESSAGE
// ============================================================
void sendTelegramAlert(String msg) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  https.begin(client, "https://api.telegram.org/bot" + BOTtoken + "/sendMessage");
  https.addHeader("Content-Type", "application/json");

  String payload = "{\"chat_id\":\"" + CHAT_ID + "\",\"text\":\"" + msg + "\"}";
  https.POST(payload);
  https.end();

  Serial.println("Telegram Alert Sent!");
}


// ============================================================
//         CHECK ACC MATCH WITH GOOGLE SHEET THRESHOLD
// ============================================================
bool fetchACCThresholdAndCheck(float A, float B, float C, float D, float E) {

  HTTPClient http;
  http.begin(SCRIPT_BASE_URL + "?type=ACC");

  int code = http.GET();
  if (code != 200) return false;

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(20000);
  deserializeJson(doc, payload);

  for (JsonObject row : doc.as<JsonArray>()) {

    float tA = row["accA"];
    float tB = row["accB"];
    float tC = row["accC"];
    float tD = row["accD"];
    float tE = row["accE"];

    if (A > tA || B > tB || C > tC || D > tD || E > tE) {

      Serial.println("---- ACC Threshold Triggered ----");
      Serial.println(String("A:") + tA + "  B:" + tB + "  C:" + tC);

      return true;
    }
  }

  return false;
}


// ============================================================
//        CHECK SYNC (ΔACC) MATCH WITH GOOGLE THRESHOLD
// ============================================================
bool fetchSYNCThresholdAndCheck(float X, float Y, float Z) {

  HTTPClient http;
  http.begin(SCRIPT_BASE_URL + "?type=SYNC");

  int code = http.GET();
  if (code != 200) return false;

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(20000);
  deserializeJson(doc, payload);

  for (JsonObject row : doc.as<JsonArray>()) {

    float tX = row["SYN_X"];
    float tY = row["SYN_Y"];
    float tZ = row["SYN_Z"];

    if (abs(X) > tX || abs(Y) > tY || abs(Z) > tZ) {

      Serial.println("---- SYNC Threshold Triggered ----");
      Serial.println(String("X:") + tX + "  Y:" + tY + "  Z:" + tZ);

      return true;
    }
  }

  return false;
}


// ============================================================
//                     SETUP
// ============================================================
void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected.");

  Wire.begin();
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("MPU6050 Connection Failed!");
    while (1);
  }

  calibrateMPU();
  sendTelegramAlert("🚀 ESP32 Started & MPU6050 Calibrated");
}


// ============================================================
//                     LOOP
// ============================================================
void loop() {

  float ax, ay, az, gx, gy, gz;
  readMPU(ax, ay, az, gx, gy, gz);

  // Compute sync (Δacc)
  float synX = ax - prevAx;
  float synY = ay - prevAy;
  float synZ = az - prevAz;

  prevAx = ax;
  prevAy = ay;
  prevAz = az;

  bool accHit  = fetchACCThresholdAndCheck(ax, ay, az, gx, gy);
  bool syncHit = fetchSYNCThresholdAndCheck(synX, synY, synZ);

  if (!alertSent && (accHit || syncHit)) {
    sendTelegramAlert("⚠️ FALL DETECTED\nThreshold Exceeded in Dataset!");
    alertSent = true;
  }

  delay(200);
}

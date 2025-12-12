#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <MPU6050.h>

// ---------------- WiFi ----------------
const char* ssid = "******************";
const char* password = "****************";

// ---------------- Telegram Bot ----------------
const String BOTtoken = "************************";
const String CHAT_ID = "*********************";

// ---------------- MPU6050 ----------------
MPU6050 mpu;

// ---- Thresholds found from your Excel dataset ----
float ACC_THRESHOLD = 6.0;   // Impact threshold from your dataset
float TILT_THRESHOLD = 60.0; // Body angle threshold

bool fallAlertSent = false;

// --------- Auto Calibration Variables ---------
float offsetAx = 0, offsetAy = 0, offsetAz = 0;

// --------- AUTO CALIBRATION FUNCTION ----------
void autoCalibrateMPU() {
  Serial.println("🛠 Starting Auto-Calibration...");
  Serial.println("Keep the device still for 3 seconds...");

  long sumAx = 0, sumAy = 0, sumAz = 0;
  const int samples = 300;  // 3 seconds @ 100Hz

  for (int i = 0; i < samples; i++) {
    int16_t ax_raw, ay_raw, az_raw;
    mpu.getAcceleration(&ax_raw, &ay_raw, &az_raw);

    sumAx += ax_raw;
    sumAy += ay_raw;
    sumAz += az_raw;

    delay(10);
  }

  offsetAx = (float)sumAx / samples;
  offsetAy = (float)sumAy / samples;
  offsetAz = ((float)sumAz / samples) - 16384.0;  // remove gravity

  Serial.println("✅ Auto-Calibration Complete!");
  Serial.print("Ax offset: "); Serial.println(offsetAx);
  Serial.print("Ay offset: "); Serial.println(offsetAy);
  Serial.print("Az offset: "); Serial.println(offsetAz);
}

// ---------------- Telegram Function ----------------
void sendTelegramAlert(String message) {
  if (WiFi.status() == WL_CONNECTED) {

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient https;
    String url = "https://api.telegram.org/bot" + BOTtoken + "/sendMessage";

    https.begin(client, url);
    https.addHeader("Content-Type", "application/json");

    String payload = "{\"chat_id\":\"" + CHAT_ID + "\",\"text\":\"" + message + "\"}";

    int httpResponseCode = https.POST(payload);

    if (httpResponseCode > 0) {
      Serial.println("Telegram message sent!");
      Serial.println(https.getString());
    } else {
      Serial.print("Error sending message: ");
      Serial.println(https.errorToString(httpResponseCode));
    }

    https.end();
  } else {
    Serial.println("WiFi not connected");
  }
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  Wire.begin();

  Serial.println("Initializing MPU6050...");
  mpu.initialize();

  if (!mpu.testConnection()) {
    Serial.println("❌ MPU6050 connection failed!");
    while (1);
  }

  Serial.println("MPU6050 connected!");

  // Start Auto Calibration
  autoCalibrateMPU();

  // Connect WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  sendTelegramAlert("🔵 ESP32 Started with Auto-Calibration Enabled!");
}

// ---------------- LOOP ----------------
void loop() {

  int16_t ax_raw, ay_raw, az_raw;
  mpu.getAcceleration(&ax_raw, &ay_raw, &az_raw);

  // Apply auto-calibrated offsets
  float ax = (ax_raw - offsetAx) / 16384.0;
  float ay = (ay_raw - offsetAy) / 16384.0;
  float az = (az_raw - offsetAz) / 16384.0;

  // ACC magnitude
  float ACC = sqrt(ax * ax + ay * ay + az * az);

  // Tilt angle
  float tilt = atan2(sqrt(ax * ax + ay * ay), abs(az)) * 57.2958;

  Serial.print("ACC: ");
  Serial.print(ACC);
  Serial.print(" | Tilt: ");
  Serial.println(tilt);

  // -------- FALL DETECTION ----------
  if (ACC > ACC_THRESHOLD && tilt > TILT_THRESHOLD) {
    if (!fallAlertSent) {
      sendTelegramAlert("🚨 FALL DETECTED!\nACC: " + String(ACC) + "\nTilt: " + String(tilt));
      fallAlertSent = true;
    }
  } else {
    fallAlertSent = false;
  }

  delay(100);
}

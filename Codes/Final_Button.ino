#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <MPU6050.h>

// ---------------- WiFi ----------------
const char* ssid = "SamsunggalaxyA35";
const char* password = "29000004";

// ---------------- Telegram Bot ----------------
const String BOTtoken = "8584168475:AAG8lYazrM8mrz5qn5zw6HDJpW_7TeMYEzU";
const String CHAT_ID = "1640526464";

// ---------------- MPU6050 ----------------
MPU6050 mpu;

// ---- Thresholds found from your Excel dataset ----
float ACC_THRESHOLD = 1.3;      // Impact threshold from your dataset
float TILT_THRESHOLD = 60.0;    // Body angle threshold

// --------- Auto Calibration Variables ---------
float offsetAx = 0, offsetAy = 0, offsetAz = 0;

// --------- Fall / Button logic variables ----------
bool fallAlertSent = false;    // whether final fall alert was sent for current event
bool fallPending = false;      // whether we're inside the 5s confirmation window
unsigned long pendingStart = 0;
const unsigned long CONFIRM_WINDOW_MS = 5000; // 5 seconds confirmation window

// cooldown to avoid repeated notifications (ms)
const unsigned long COOLDOWN_MS = 7000;
unsigned long lastHandledTime = 0;

// --------- Push button config (change pin if desired) ----------
const int BUTTON_PIN = 17;      // GPIO pin for cancel button (change if you prefer)
const int BUTTON_ACTIVE_LEVEL = LOW; // using INPUT_PULLUP, so pressed reads LOW
const unsigned long DEBOUNCE_MS = 50;
unsigned long lastButtonChange = 0;
int lastButtonState = HIGH; // because INPUT_PULLUP idle is HIGH

// --------- TELEGRAM helper ----------
void sendTelegramAlert(const String &message) {
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

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(BUTTON_PIN, INPUT_PULLUP); // button connected to GND, internal pullup enabled

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
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi");
  sendTelegramAlert("🔵 ESP32 Started with Auto-Calibration Enabled!");
}

// read debounced button state; returns true if pressed now
bool isButtonPressed() {
  int raw = digitalRead(BUTTON_PIN);
  if (raw != lastButtonState) {
    lastButtonChange = millis();
    lastButtonState = raw;
  }
  if ((millis() - lastButtonChange) > DEBOUNCE_MS) {
    // stable
    return (raw == BUTTON_ACTIVE_LEVEL);
  }
  return false;
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
  Serial.print(ACC, 3);
  Serial.print(" | Tilt: ");
  Serial.println(tilt, 2);

  unsigned long now = millis();

  // If we're currently pending confirmation (5s window)
  if (fallPending) {
    // If user pressed the cancel button during pending
    if (isButtonPressed()) {
      fallPending = false;
      fallAlertSent = false; // no final alert
      lastHandledTime = now;
      Serial.println("❎ False alarm — cancelled by user button");
      sendTelegramAlert("✅ False alarm cancelled by user (button pressed).");
    } else if (now - pendingStart >= CONFIRM_WINDOW_MS) {
      // confirmation window expired -> send final alert if not already sent
      fallPending = false;
      if (!fallAlertSent) {
        fallAlertSent = true;
        lastHandledTime = now;
        String msg = "🚨 FALL DETECTED!\nACC: " + String(ACC, 3) + "\nTilt: " + String(tilt, 2);
        Serial.println("‼️ Final fall alert sent.");
        sendTelegramAlert(msg);
      }
    }
    // while pending, skip detection logic to avoid restarts
    delay(50); // small delay to reduce loop churn while pending
    return;
  }

  // Not pending: check for new potential fall (respect cooldown)
  if ((now - lastHandledTime) > COOLDOWN_MS) {
    if (ACC > ACC_THRESHOLD && tilt > TILT_THRESHOLD) {
      // start pending confirmation window
      fallPending = true;
      pendingStart = now;
      Serial.println("⚠️ Potential fall detected — waiting 5s for cancel button...");
      // Inform remote user there's a potential fall and they have 5s to cancel
      String warn = "⚠️ Potential fall detected — press CANCEL button within 5 seconds to stop the alert.\n(If not cancelled, a fall alert will be sent.)\nACC: " + String(ACC, 3) + "\nTilt: " + String(tilt, 2);
      sendTelegramAlert(warn);
    }
  }

  // small loop delay
  delay(200);
}

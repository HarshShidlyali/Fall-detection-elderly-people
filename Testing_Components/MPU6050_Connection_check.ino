// This code is to check the connection between the ESP32 and MPU6050 sensor to ensure the correct workings. 

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // Wait for serial port to connect
  }
  
  Serial.println("MPU6050 Connection Test");
  Serial.println("=======================");
  
  // Initialize I2C communication
  Wire.begin();
  
  // Try to initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("❌ Failed to find MPU6050 chip!");
    Serial.println("Please check the following:");
    Serial.println("1. Wiring connections (SDA, SCL, VCC, GND)");
    Serial.println("2. I2C address (try 0x68 or 0x69)");
    Serial.println("3. Power supply");
    
    // Scan I2C bus to see what devices are available
    scanI2CDevices();
    
    while (1) {
      delay(1000);
    }
  }
  
  Serial.println("✅ MPU6050 Found!");
  
  // Set accelerometer range
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
    case MPU6050_RANGE_2_G:
      Serial.println("±2 G");
      break;
    case MPU6050_RANGE_4_G:
      Serial.println("±4 G");
      break;
    case MPU6050_RANGE_8_G:
      Serial.println("±8 G");
      break;
    case MPU6050_RANGE_16_G:
      Serial.println("±16 G");
      break;
  }
  
  // Set gyro range
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
    case MPU6050_RANGE_250_DEG:
      Serial.println("±250 deg/s");
      break;
    case MPU6050_RANGE_500_DEG:
      Serial.println("±500 deg/s");
      break;
    case MPU6050_RANGE_1000_DEG:
      Serial.println("±1000 deg/s");
      break;
    case MPU6050_RANGE_2000_DEG:
      Serial.println("±2000 deg/s");
      break;
  }
  
  // Set filter bandwidth
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
    case MPU6050_BAND_260_HZ:
      Serial.println("260 Hz");
      break;
    case MPU6050_BAND_184_HZ:
      Serial.println("184 Hz");
      break;
    case MPU6050_BAND_94_HZ:
      Serial.println("94 Hz");
      break;
    case MPU6050_BAND_44_HZ:
      Serial.println("44 Hz");
      break;
    case MPU6050_BAND_21_HZ:
      Serial.println("21 Hz");
      break;
    case MPU6050_BAND_10_HZ:
      Serial.println("10 Hz");
      break;
    case MPU6050_BAND_5_HZ:
      Serial.println("5 Hz");
      break;
  }
  
  Serial.println("\n✅ MPU6050 initialized successfully!");
  delay(1000);
}

void loop() {
  // Get sensor events
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  
  // Print sensor data
  Serial.println("\n--- MPU6050 Sensor Data ---");
  Serial.print("Temperature: "); Serial.print(temp.temperature); Serial.println(" °C");
  
  Serial.println("Acceleration (m/s²):");
  Serial.print("X: "); Serial.print(a.acceleration.x);
  Serial.print(" | Y: "); Serial.print(a.acceleration.y);
  Serial.print(" | Z: "); Serial.print(a.acceleration.z); Serial.println();
  
  Serial.println("Rotation (rad/s):");
  Serial.print("X: "); Serial.print(g.gyro.x);
  Serial.print(" | Y: "); Serial.print(g.gyro.y);
  Serial.print(" | Z: "); Serial.print(g.gyro.z); Serial.println();
  
  delay(2000);
}

void scanI2CDevices() {
  Serial.println("\n🔍 Scanning I2C bus...");
  byte error, address;
  int nDevices = 0;
  
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
      
      nDevices++;
      
      // Check if it's MPU6050
      if (address == 0x68 || address == 0x69) {
        Serial.println("⚠️  This might be an MPU6050! Check your wiring.");
      }
    }
  }
  
  if (nDevices == 0) {
    Serial.println("No I2C devices found. Please check wiring!");
  } else {
    Serial.println("I2C scan complete.");
  }
}

#include <Arduino.h>
#include <DHT.h>
#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <TinyGPS++.h>
#include <math.h>

// pin configuration
#define DHT_PIN     5
#define DHTTYPE     DHT11

#define TRIG_PIN    13
#define ECHO_PIN    35

#define LED_PIN     2
#define BUZZER_PIN  27
#define FLAME_PIN   32

#define MPU_SDA     21
#define MPU_SCL     22

#define SOIL_PIN    33

#define GPS_RX      16   
#define GPS_TX      17

#define LORA_SS     4
#define LORA_RST    14
#define LORA_DIO0   26

// objects with lib. & data type
DHT dht(DHT_PIN, DHTTYPE);
Adafruit_MPU6050 mpu;
TinyGPSPlus gps;
bool mpuOK = false;

// Risk factor level
const float HIGH_RISK_CM = 5.0f;
const float TILT_LIMIT = 30.0f;     // 30* slope
const float SOIL_LIMIT = 80.0f;     // 80% moist.
const float LORA_FREQ = 433E6;      // frequency rate

// Function decleration
float readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long echoTime = pulseIn(ECHO_PIN, HIGH, 30000UL); // 30 ms timeout
  if (echoTime == 0) return -1.0f;

  return (echoTime * 0.0343f) / 2.0f;
}
// blink during transmiting packets by
void blinkLedDuringTransmit() {
  digitalWrite(LED_PIN, HIGH);
  delay(50);
  digitalWrite(LED_PIN, LOW);
}

void triggerAlarm() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(2000);
  digitalWrite(BUZZER_PIN, LOW);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- Initializing Sensor Node ---");

  // DHT11
  dht.begin();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(FLAME_PIN, INPUT_PULLUP);

  digitalWrite(TRIG_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Soil sensor
  analogSetPinAttenuation(SOIL_PIN, ADC_11db);

  // MPU6050
  Wire.begin(MPU_SDA, MPU_SCL);
  Serial.println("Looking for MPU6050...");

  if (mpu.begin(0x68, &Wire)) {
    Serial.println("MPU6050 found at address 0x68");
    mpuOK = true;
  } else if (mpu.begin(0x69, &Wire)) {
    Serial.println("MPU6050 found at address 0x69");
    mpuOK = true;
  } else {
    Serial.println("ERROR: MPU6050 not responding");
    mpuOK = false;
  }

  if (mpuOK) {
    mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }

  // GPS
  Serial2.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.println("GPS initialized");

  // LoRa
  SPI.begin();  // SCK=18, MISO=19, MOSI=23
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  Serial.println("Initializing LoRa...");
  while (!LoRa.begin(LORA_FREQ)) {
    Serial.println("LoRa init failed, retrying");
    delay(500);
  }
  Serial.println("LoRa init OK");

  Serial.println(" System Setup Complete \n");
}

void loop() {
  // Read DHT11
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Read Water Distance
  float distance = readDistanceCM();

  // Flame Sensor
  bool flameDetected = (digitalRead(FLAME_PIN) == LOW);

  // MPU6050 Tilt 
  float roll = 0.0f;
  float pitch = 0.0f;
  float tilt_mag = 0.0f;

  if (mpuOK) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    roll = atan2f(a.acceleration.y, a.acceleration.z) * 180.0f / PI;
    pitch = atan2f(
      -a.acceleration.x,
      sqrtf(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)
    ) * 180.0f / PI;

    tilt_mag = sqrtf(roll * roll + pitch * pitch);
  }

  //  Soil Moisture 
  int soilRaw = analogRead(SOIL_PIN);
  float soilPerc = (4095.0f - soilRaw) * 100.0f / 4095.0f;  // adjust after calibration if needed

  //  GPS 
  while (Serial2.available() > 0) {
    gps.encode(Serial2.read());
  }

  bool gpsValid = gps.location.isValid();
  double latitude = gpsValid ? gps.location.lat() : 0.0;
  double longitude = gpsValid ? gps.location.lng() : 0.0;

  //  Hazard Condition
  bool hazard = false;
  if (tilt_mag > 1.5f) hazard = true;
  if (flameDetected) hazard = true;
  if (soilPerc > SOIL_LIMIT) hazard = true;
  if (distance > 0 && distance < 20.0f) hazard = true;

  //  Print readings 
  Serial.println("----- Sensor Readings -----");

  Serial.print("Temperature: ");
  Serial.println(isnan(temperature) ? -99.0f : temperature);

  Serial.print("Humidity: ");
  Serial.println(isnan(humidity) ? -99.0f : humidity);

  Serial.print("Roll: ");
  Serial.println(roll);

  Serial.print("Pitch: ");
  Serial.println(pitch);

  Serial.print("Tilt Magnitude: ");
  Serial.println(tilt_mag);

  Serial.print("Distance: ");
  Serial.println(distance);

  Serial.print("Soil Moisture: ");
  Serial.println(soilPerc);

  Serial.print("Flame Detected: ");
  Serial.println(flameDetected ? "YES" : "NO");

  Serial.print("GPS Valid: ");
  Serial.println(gpsValid ? "YES" : "NO");

  Serial.print("Latitude: ");
  Serial.println(latitude, 6);

  Serial.print("Longitude: ");
  Serial.println(longitude, 6);

  Serial.print("Hazard: ");
  Serial.println(hazard ? "YES" : "NO");

  Serial.println("--------------------------");

  // ----- Send via LoRa -----
  // temp,humidity,roll,pitch,tilt_mag,distance,soilPerc,flame,gpsValid,lat,lng,hazard
  char payload[220];
snprintf(
  payload, sizeof(payload),
  "temp=%.1f,humidity=%.1f,roll=%.2f,pitch=%.2f,tilt=%.2f,distance=%.1f,soil=%.1f,flame=%d,gpsValid=%d,lat=%.6f,lng=%.6f,hazard=%d",
  isnan(temperature) ? -99.0f : temperature,
  isnan(humidity) ? -99.0f : humidity,
  roll,
  pitch,
  tilt_mag,
  distance,
  soilPerc,
  flameDetected ? 1 : 0,
  gpsValid ? 1 : 0,
  latitude,
  longitude,
  hazard ? 1 : 0
);

Serial.print("Sending: ");
Serial.println(payload);

blinkLedDuringTransmit();
LoRa.beginPacket();
LoRa.print(payload);
LoRa.endPacket();

  // Alarm on hazard 
  if (hazard) {
    triggerAlarm();
  }

  Serial.println("----------------------------");
  delay(1000);
}
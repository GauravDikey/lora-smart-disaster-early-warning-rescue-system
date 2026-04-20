#include <SPI.h>
#include <LoRa.h>
#include <LiquidCrystal.h>
#include <HardwareSerial.h>

// lora pin configuration
#define LORA_SS   4
#define LORA_RST  14
#define LORA_DIO0 26
#define LORA_SCK  18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_FREQ 433E6

// display pins config.
LiquidCrystal lcd(27, 25, 33, 32, 21, 22);

//  SIM800L Pins
HardwareSerial sim800(2); 
#define SIM800_RX 16       
#define SIM800_TX 17      

String phoneNumber = "+918767716320"; 

// Sensor Data
String lastMessage = "";
int lastRSSI = 0;
float lastSNR = 0.0;

float temperature = -99.0;
float humidity    = -99.0;
float roll        = 0.0;
float pitch       = 0.0;
float tilt_mag    = 0.0;
float distanceVal = 0.0;
float soilPerc    = 0.0;
int flameDetected = 0;
int gpsValid      = 0;
float latitude    = 0.0;
float longitude   = 0.0;
int hazard        = 0;

// Display Control
unsigned long lastPageChange = 0;
const unsigned long PAGE_INTERVAL_MS = 2000;
byte lcdPage = 0;

// Alarm / SMS Status
bool hazardLatched = false;
unsigned long lastSmsTime = 0;
const unsigned long SMS_COOLDOWN_MS = 60000;

// SIM800 Helpers
void sendCommand(String cmd, int waitMs = 1000) {
  sim800.println(cmd);
  delay(waitMs);
  while (sim800.available()) {
    Serial.write(sim800.read());
  }
}

bool initSIM800() {
  sim800.begin(9600, SERIAL_8N1, SIM800_RX, SIM800_TX);
  delay(3000);

  Serial.println("Initializing SIM800L...");
  sendCommand("AT", 1000);
  sendCommand("ATE0", 1000);
  sendCommand("AT+CMGF=1", 1000);
  return true;
}

bool sendSMS(String number, String message) {
  sendCommand("AT", 500);
  sendCommand("ATE0", 500);
  sendCommand("AT+CMGF=1", 500);

  sim800.print("AT+CMGS=\"");
  sim800.print(number);
  sim800.println("\"");
  delay(1000);

  sim800.print(message);
  sim800.write(26); 
  delay(5000);

  bool gotResponse = false;
  while (sim800.available()) {
    gotResponse = true;
    Serial.write(sim800.read());
  }

  return gotResponse;
}

// Payload Parsing
String getValue(String data, String key) {
  int start = data.indexOf(key + "=");
  if (start == -1) return "";

  start += key.length() + 1;
  int end = data.indexOf(",", start);
  if (end == -1) end = data.length();

  return data.substring(start, end);
}

void parseData(String msg) {
  String v;

  v = getValue(msg, "temp");      if (v != "") temperature = v.toFloat();
  v = getValue(msg, "humidity");  if (v != "") humidity = v.toFloat();
  v = getValue(msg, "roll");      if (v != "") roll = v.toFloat();
  v = getValue(msg, "pitch");     if (v != "") pitch = v.toFloat();
  v = getValue(msg, "tilt");      if (v != "") tilt_mag = v.toFloat();
  v = getValue(msg, "distance");  if (v != "") distanceVal = v.toFloat();
  v = getValue(msg, "soil");      if (v != "") soilPerc = v.toFloat();
  v = getValue(msg, "flame");     if (v != "") flameDetected = v.toInt();
  v = getValue(msg, "gpsValid");  if (v != "") gpsValid = v.toInt();
  v = getValue(msg, "lat");       if (v != "") latitude = v.toFloat();
  v = getValue(msg, "lng");       if (v != "") longitude = v.toFloat();
  v = getValue(msg, "hazard");    if (v != "") hazard = v.toInt();
}

// Hazard Condition
bool isHazardMessage(String msg) {
  msg.toUpperCase();

  if (msg.indexOf("HAZARD") >= 0) return true;
  if (msg.indexOf("ALERT") >= 0) return true;
  if (msg.indexOf("DANGER") >= 0) return true;
  if (msg.indexOf("HIGH") >= 0) return true;
  if (msg.indexOf("FIRE") >= 0) return true;
  if (msg.indexOf("WATER HIGH") >= 0) return true;
  if (msg.indexOf("HAZARD=1") >= 0) return true;

  return false;
}

String buildRescueMessage() {
  String sms = "RESCUE ALERT!\n";
  sms += "Hazard detected via LoRa.\n";
  sms += "Data: " + lastMessage + "\n";
  sms += "RSSI: " + String(lastRSSI) + "\n";
  sms += "SNR: " + String(lastSNR, 1) + "\n";
  sms += "Lat: " + String(latitude, 6) + "\n";
  sms += "Lng: " + String(longitude, 6) + "\n";
  sms += "Please respond immediately.";
  return sms;
}

void triggerRescueSMS() {
  unsigned long now = millis();

  if (now - lastSmsTime < SMS_COOLDOWN_MS) {
    Serial.println("SMS cooldown active, skipping send.");
    return;
  }

  String sms = buildRescueMessage();
  Serial.println("Sending rescue SMS...");
  Serial.println(sms);

  bool ok = sendSMS(phoneNumber, sms);
  if (ok) {
    Serial.println("SMS send attempt completed.");
    lastSmsTime = now;
  } else {
    Serial.println("SMS send may have failed.");
  }
}

// LCD Display 
void showPage() {
  char line1[17];
  char line2[17];

  lcd.clear();

  switch (lcdPage) {
    case 0:
      snprintf(line1, sizeof(line1), "T:%5.1f H:%5.1f", temperature, humidity);
      snprintf(line2, sizeof(line2), "Temp/Humidity");
      break;

    case 1:
      snprintf(line1, sizeof(line1), "R:%5.2f P:%5.2f", roll, pitch);
      snprintf(line2, sizeof(line2), "Roll/Pitch");
      break;

    case 2:
      snprintf(line1, sizeof(line1), "Tilt:%5.2f", tilt_mag);
      snprintf(line2, sizeof(line2), "Dist:%5.1f", distanceVal);
      break;

    case 3:
      snprintf(line1, sizeof(line1), "Soil:%5.1f", soilPerc);
      snprintf(line2, sizeof(line2), "Flame:%d", flameDetected);
      break;

    case 4:
      snprintf(line1, sizeof(line1), "GPS:%d Hazard:%d", gpsValid, hazard);
      snprintf(line2, sizeof(line2), "Alert Status");
      break;

    case 5:
      snprintf(line1, sizeof(line1), "Lat:%8.4f", latitude);
      snprintf(line2, sizeof(line2), "Lng:%8.4f", longitude);
      break;

    case 6:
      snprintf(line1, sizeof(line1), "RSSI:%d", lastRSSI);
      snprintf(line2, sizeof(line2), "SNR:%4.1f", lastSNR);
      break;
  }

  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  delay(1000);

  // LCD init
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Starting...");
  lcd.setCursor(0, 1);
  lcd.print("LCD + LoRa");

  // SIM800 init
  initSIM800();

  // LoRa SPI init
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  Serial.println("Starting LoRa RX...");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Starting LoRa");
  lcd.setCursor(0, 1);
  lcd.print("Receiver...");

  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("LoRa init failed");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("LoRa init fail");
    while (1) {
      delay(1000);
    }
  }

  LoRa.receive();
  Serial.println("LoRa Receiver Ready");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LoRa Ready");
  lcd.setCursor(0, 1);
  lcd.print("Waiting...");
  delay(1500);
  showPage();
}

//  Main Loop 
void loop() {
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    String incoming = "";

    while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }
    incoming.trim();

    lastMessage = incoming;
    lastRSSI = LoRa.packetRssi();
    lastSNR = LoRa.packetSnr();

    Serial.println("----- Packet Received -----");
    Serial.print("Message: ");
    Serial.println(lastMessage);
    Serial.print("RSSI: ");
    Serial.println(lastRSSI);
    Serial.print("SNR: ");
    Serial.println(lastSNR);
    Serial.println("---------------------------");

    parseData(lastMessage);

    bool hazardNow = isHazardMessage(lastMessage);

    if (hazardNow && !hazardLatched) {
      Serial.println("Hazard detected!");
      hazardLatched = true;
      triggerRescueSMS();
    }

    if (!hazardNow && hazardLatched) {
      Serial.println("Hazard cleared.");
      hazardLatched = false;
    }

    showPage();
    LoRa.receive();
  }

  if (millis() - lastPageChange >= PAGE_INTERVAL_MS) {
    lastPageChange = millis();
    lcdPage++;
    if (lcdPage > 6) lcdPage = 0;
    showPage();
  }
}
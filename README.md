# LoRa-based Smart Disaster Early Warning & Rescue Assistance System

A solar-powered, ESP32-based disaster monitoring and rescue assistance system designed to detect environmental risks such as rising water level, slope tilt, forest fire, soil moisture changes, temperature, humidity, and GPS location.  
The sensor node sends real-time readings and alerts through LoRa to a base station, where the data is displayed on an LCD1602 screen and emergency SMS alerts can be sent using a SIM800L module.

---

## Project Objective

The main goal of this project is to build a low-cost, long-range, off-grid early warning system for disaster-prone areas such as:

- riverbanks
- hills and landslide-prone slopes
- forest regions
- remote villages
- flood-prone zones

The system is designed to work even where Wi-Fi or internet is unavailable.

---

## Main Features

- Real-time water level monitoring using HC-SR04
- Landslide / slope tilt detection using MPU6050
- Forest fire detection using flame sensor
- Soil moisture monitoring for dry, wet, and muddy conditions
- Temperature and humidity monitoring using DHT11
- GPS location tracking using NEO-6M
- Long-range wireless communication using LoRa
- Local LED and buzzer alert on hazard detection
- Base station display using LCD1602 with I2C interface
- Emergency SMS alert using SIM800L
- Solar-powered battery operation for outdoor use

---

## System Overview

This project contains two major parts:

### 1. Sensor Node
The sensor node is placed in the field and continuously monitors environmental conditions.  
It collects sensor data, evaluates hazard thresholds, and sends the readings to the base station through LoRa.

### 2. Base Station
The base station receives LoRa packets from the node, shows live readings on the LCD1602 display, and sends SMS alerts to a mobile phone if danger is detected.

---

## Working Principle

1. The ESP32 sensor node reads all connected sensors.
2. It checks whether any hazard condition exists.
3. If values are normal, it sends periodic status updates.
4. If a danger threshold is crossed, the node activates:
   - LED indicator
   - buzzer alarm
   - LoRa alert transmission
5. The base station receives the packet and:
   - displays the values on LCD1602
   - sends SMS alert using SIM800L
   - can be extended for Wi-Fi logging or cloud monitoring

---

## Hardware Used

### Sensor Node
- ESP32 Dev Board
- LoRa SX1278 ra-02 module
- MPU6050
- NEO-6M GPS
- HC-SR04 ultrasonic sensor
- Flame sensor
- DHT11 sensor
- Soil moisture sensor
- LED
- Buzzer
- Solar panel
- 2 × 18650 battery pack
- Battery charger / BMS
- Buck converter
- Resistors, capacitors, diodes, and connecting wires

### Base Station
- ESP32 Dev Board
- LoRa SX1276 / RFM95 module
- LCD1602 with I2C backpack
- SIM800L GSM module
- LED / buzzer if needed
- Power supply
- Connecting wires and supporting components

---

## Pin Configuration

### Sensor Node
- DHT11 → GPIO5
- LED → GPIO2
- Buzzer → GPIO27
- MPU6050 → I2C SDA-GPIO21 , SCL-GPIO22
- GPS NEO-6M → RX-GPIO16 , TX-GPIO17
- HC-SR04 → TRIG GPIO13, ECHO GPIO35
- Soil moisture sensor → GPIO33
- Flame sensor → GPIO32
- LoRa SX1276 → SCK GPIO18, MISO GPIO19, MOSI GPIO23, NSS GPIO4, RST GPIO14, DIO0 GPIO26

### Base Station
- LoRa SX1276 → same SPI pins as sensor node
- LCD1602 I2C → GPIO ( 27,25,33,32,21,22 )
- SIM800L → RX-16 , TX-17

---

## Alert Conditions

The system triggers an alert when any of the following happens:

- water level rises too much
- tilt angle indicates possible landslide
- flame is detected
- soil moisture indicates muddy / risky condition
- abnormal temperature or humidity is observed

When an alert happens:
- LED turns ON
- buzzer sounds for 2 seconds
- LoRa sends the alert packet
- base station receives the packet
- LCD displays the warning
- SIM800L sends SMS to the selected mobile number

---

## Software Requirements

- Arduino IDE
- ESP32 board package
- Required libraries:
  - LoRa
  - Wire
  - TinyGPS++
  - DHT sensor library
  - Adafruit MPU6050
  - LiquidCrystal_I2C
  - TinyGSM

---

## How to Build

### Step 1: Prepare the Hardware
Assemble the node and base station on a breadboard first.  
Test each module separately before integrating all components.

### Step 2: Upload Node Code
Upload the sensor node sketch to the ESP32 connected to all sensors.

### Step 3: Upload Base Station Code
Upload the base station sketch to another ESP32 connected to LoRa, LCD1602, and SIM800L.

### Step 4: Test Communication
Power both units and verify:
- sensor readings on Serial Monitor
- LoRa packet transmission
- LCD display on base station
- SMS alert on risky conditions

### Step 5: Field Testing
Test the system outdoors with solar power and confirm:
- stable battery charging
- long-range LoRa communication
- correct hazard alerts
- GPS location reporting

---

## Project Flow

### Sensor Node
Sense → Process → Detect hazard → Alert locally → Send LoRa data

### Base Station
Receive LoRa → Parse data → Show on LCD → Send SMS if needed

---

## Example Use Cases

- flood warning system near rivers
- landslide monitoring on hills
- forest fire early warning
- disaster rescue support for remote villages
- environmental monitoring for agricultural land

---

## Future Improvements

- Add OLED or TFT graphical display
- Add cloud dashboard
- Add mobile app integration
- Add better soil moisture sensing
- Add multiple nodes in one network
- Use battery management telemetry
- Improve low-power sleep modes for longer runtime

---

## Notes

- LoRa modules must be powered with 3.3V only
- HC-SR04 echo must be level-shifted to 3.3V before ESP32 input
- SIM800L needs a stable power supply with enough current
- Use protected 18650 cells and proper BMS for safety
- Keep antenna connected during LoRa testing

---

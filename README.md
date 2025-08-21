# 🌦️ Weather Station with Arduino + Raspberry Pi + ThingSpeak

## 📌 Description

This project implements an **experimental weather station** composed of an **Arduino** responsible for collecting environmental data and a **Raspberry Pi** in charge of periodically transmitting this information to **ThingSpeak**, where the data can be analyzed and visualized in real time.

The solution was designed to work **robustly and autonomously**, ensuring:

* Reliable reading of multiple sensors.
* Controlled interval uploads to the cloud.
* Safe opening and closing of the serial port on each cycle, avoiding lock-ups.

---

## ⚙️ Architecture

### 🟦 Arduino

The Arduino is responsible for:

* **Collecting data from sensors**:

  * Indoor Temperature (DS18B20 or DHT)
  * Outdoor Temperature (DS18B20)
  * Atmospheric Pressure (BMP180 / BMP085)
  * Relative Humidity (DHT11/DHT22)
  * Dew Point (calculated)
  * CAPE (Convective Available Potential Energy, calculated)
  * Pressure Trend

* **Formatting the data** into a single CSV line:

  ```
  TempIN,TempOUT,Pressure,Humidity,DewPoint,CAPE,PressureTrend
  ```

* **Continuously sending via serial** to the Raspberry Pi.

---

### 🟥 Raspberry Pi

The Raspberry Pi runs a **Python script** that acts as an **intermediary** between the Arduino and ThingSpeak:

1. **Every 80 seconds**:

   * Opens the Arduino serial port.
   * Reads several lines and selects the **last valid line** received.
   * Closes the serial port.

2. **Processing**:

   * Converts the line into floats.
   * Rounds the values to 2 decimal places.
   * Prints each parameter with its label in the terminal (e.g., `Indoor Temperature: 26.00 °C`).

3. **Upload to ThingSpeak**:

   * Builds an HTTP payload with the **7 configured fields**.
   * Sends the request via `requests.get()` to the ThingSpeak API.
   * Waits 80 seconds and repeats the cycle.

---

## 🚀 Workflow Summary

1. Arduino **reads sensors continuously** and writes values to the serial port.
2. Every 80s, the Raspberry Pi **opens the serial, captures the last line, closes the serial**.
3. Data is automatically sent to **ThingSpeak**.
4. The user can monitor real-time graphs on the ThingSpeak dashboard.

---

## 📊 ThingSpeak Fields

* Field 1 → Indoor Temperature
* Field 2 → Outdoor Temperature
* Field 3 → Pressure
* Field 4 → Humidity
* Field 5 → Dew Point
* Field 6 → CAPE
* Field 7 → Pressure Trend

---

## 🔧 Dependencies

* **Arduino** with sensor libraries (`DHT`, `DallasTemperature`, `Adafruit_BMP085_U`, etc.).
* **Python 3** on Raspberry Pi with the following packages:

  ```bash
  pip install pyserial requests
  ```

---

## ✅ Highlights

* Serial opened and closed every cycle (avoids lock-ups).
* Clean synchronization between Arduino → Raspberry Pi → ThingSpeak.
* Scalable: new sensors can be added easily.
* Online visualization in ThingSpeak.

---

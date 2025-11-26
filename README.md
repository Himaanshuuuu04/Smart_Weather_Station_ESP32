# 🌤️ ESP32 Smart Weather Station

> An IoT-powered weather monitoring system with **Gemini AI health advisories** and **Telegram notifications**

![ESP32](https://img.shields.io/badge/ESP32-IoT-blue)
![Gemini AI](https://img.shields.io/badge/Gemini-AI%20Powered-orange)
![Telegram](https://img.shields.io/badge/Telegram-Bot%20Alerts-blue)
![License](https://img.shields.io/badge/License-MIT-green)

## 📋 Overview

This project transforms an ESP32 microcontroller into a **smart weather station** that:

- 📊 Monitors **indoor temperature and humidity** using a DHT11 sensor
- 🌍 Fetches **real-time outdoor weather data** for Noida, India
- 🤖 Generates **AI-powered health advisories** using Google's Gemini API
- 📱 Sends **instant Telegram alerts** when conditions exceed thresholds
- 🌐 Hosts a **responsive web dashboard** for real-time monitoring

---

## ✨ Features

### 🏠 Indoor Monitoring
- Real-time temperature and humidity readings from DHT11 sensor
- 5-second refresh interval for accurate tracking
- Threshold-based alert system

### 🌍 Outdoor Weather Integration
- Live weather data from Open-Meteo API (Noida, India)
- Tracks: Temperature, Humidity, Feels-like, Wind Speed, UV Index
- Weather condition descriptions with emoji indicators
- Auto-refresh every 10 minutes

### 🤖 Gemini AI Health Advisor
- Personalized health recommendations based on indoor + outdoor conditions
- Creative, context-aware advice including:
  - 👕 Clothing suggestions
  - 💧 Hydration reminders
  - 🏃 Activity recommendations
  - 🫁 Respiratory health tips
  - 😴 Sleep quality advice

### 📱 Telegram Bot Notifications
- **Instant alerts** when thresholds are exceeded:
  - Humidity ≥ 80%
  - Temperature ≥ 35°C
  - Temperature ≤ 15°C
- **AI-enhanced messages** with personalized precautions
- **Recovery notifications** when conditions normalize
- **Startup notification** with initial health tip

### 🖥️ Web Dashboard
- Clean, minimalist black & white design
- Real-time sensor data display
- Outdoor weather panel
- AI advisory section
- Historical weather data (14-day)
- Responsive layout for all devices

---

## 🛠️ Hardware Requirements

| Component | Quantity | Description |
|-----------|----------|-------------|
| ESP32 Dev Board | 1 | Main microcontroller |
| DHT11 Sensor | 1 | Temperature & humidity sensor |
| Jumper Wires | 3 | For connections |
| USB Cable | 1 | For power and programming |

### 📌 Wiring Diagram

```
ESP32          DHT11
------         ------
3.3V    →      VCC
GND     →      GND
GPIO23  →      DATA
```

---

## 📦 Software Dependencies

Install the following libraries in Arduino IDE:

```
- WiFi.h (Built-in)
- WebServer.h (Built-in)
- WiFiClientSecure.h (Built-in)
- HTTPClient.h (Built-in)
- ArduinoJson (by Benoit Blanchon) - v6.x or v7.x
- DHT sensor library (by Adafruit)
```

### Installation via Arduino Library Manager:
1. Open Arduino IDE
2. Go to **Sketch → Include Library → Manage Libraries**
3. Search and install:
   - `ArduinoJson` by Benoit Blanchon
   - `DHT sensor library` by Adafruit

---

## ⚙️ Configuration

### 1. WiFi Credentials
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

### 2. Telegram Bot Setup

1. **Create a Bot:**
   - Message [@BotFather](https://t.me/BotFather) on Telegram
   - Send `/newbot` and follow instructions
   - Copy the bot token provided

2. **Get Your Chat ID:**
   - Message [@userinfobot](https://t.me/userinfobot) on Telegram
   - It will reply with your chat ID

3. **Update the code:**
```cpp
const char* telegramBotToken = "YOUR_BOT_TOKEN";
const char* telegramChatId = "YOUR_CHAT_ID";
```

### 3. Gemini API Key

1. Visit [Google AI Studio](https://aistudio.google.com/)
2. Create an API key
3. Update the code:
```cpp
const char* geminiApiKey = "YOUR_GEMINI_API_KEY";
```

### 4. Location Configuration (Optional)
Default is set to Noida, India. Modify for your location:
```cpp
const float NOIDA_LAT = 28.5355;  // Your latitude
const float NOIDA_LON = 77.3910;  // Your longitude
```

---

## 🚀 Installation & Upload

1. **Clone this repository:**
```bash
git clone https://github.com/YOUR_USERNAME/esp32-smart-weather-station.git
```

2. **Open in Arduino IDE:**
   - Open `esp32_smart_weather_station.ino`

3. **Select Board:**
   - Go to **Tools → Board → ESP32 Arduino → ESP32 Dev Module**

4. **Select Port:**
   - Go to **Tools → Port → COMx** (your ESP32 port)

5. **Upload:**
   - Click the Upload button (→)

6. **Access Dashboard:**
   - Open Serial Monitor (115200 baud)
   - Note the IP address displayed
   - Open `http://[IP_ADDRESS]` in your browser

---

## 🌐 Web Dashboard

### Main Features:

| Section | Description |
|---------|-------------|
| **Indoor Sensor** | Live DHT11 temperature & humidity readings |
| **Outdoor Weather** | Real-time Noida weather data |
| **AI Advisory** | On-demand Gemini AI health recommendations |
| **Historical Data** | 14-day weather trends for your location |
| **Telegram Alerts** | Alert status and test functionality |

### Auto-Geolocation
The dashboard can auto-detect your location for historical weather data. For this to work over HTTP:

1. Open Chrome and go to: `chrome://flags/#unsafely-treat-insecure-origin-as-secure`
2. Add your ESP32's IP (e.g., `http://192.168.1.100`)
3. Enable and restart Chrome

---

## 📱 Telegram Alert Format

### 🚨 Alert Message Example:
```
🚨 SMART ALERT: HIGH HUMIDITY
━━━━━━━━━━━━━━━━━━━━

🏠 INDOOR CONDITIONS
├ 🌡️ Temperature: 28.5°C
└ 💧 Humidity: 85%

🌍 OUTDOOR (NOIDA)
├ 🌡️ Temperature: 32.0°C
├ 💧 Humidity: 70%
├ 🌤️ Condition: Partly cloudy ⛅
├ 🌬️ Wind: 12.5 km/h
└ ☀️ UV Index: 6.5

━━━━━━━━━━━━━━━━━━━━
🤖 AI HEALTH ADVISORY
━━━━━━━━━━━━━━━━━━━━

[Personalized AI recommendations here]

━━━━━━━━━━━━━━━━━━━━
📍 ESP32 Smart Weather Station
```

---

## 🔧 API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Main web dashboard |
| `/readings` | GET | Indoor sensor data (JSON) |
| `/outdoor` | GET | Noida outdoor weather (JSON) |
| `/api` | GET | Historical weather data (JSON) |
| `/setLocation` | POST | Set location for historical data |
| `/testAlert` | GET | Send test Telegram alert with AI advice |

### Example Response - `/readings`:
```json
{
  "temperature": 27.5,
  "humidity": 65.0,
  "valid": true,
  "uptime": 3600,
  "alertActive": false
}
```

---

## 📊 Alert Thresholds

| Condition | Threshold | Action |
|-----------|-----------|--------|
| High Humidity | ≥ 80% | Telegram alert + AI advice |
| High Temperature | ≥ 35°C | Telegram alert + AI advice |
| Low Temperature | ≤ 15°C | Telegram alert + AI advice |
| Recovery | Below thresholds | Normalization notification |

**Cooldown:** 5 minutes between alerts to prevent spam

---

## 🏗️ Project Structure

```
esp32-smart-weather-station/
├── esp32_smart_weather_station.ino   # Main Arduino code
├── README.md                          # This documentation
└── LICENSE                            # MIT License
```

---

## 🔌 System Architecture

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   DHT11     │────▶│    ESP32    │────▶│  Web UI     │
│   Sensor    │     │   (WiFi)    │     │  Dashboard  │
└─────────────┘     └──────┬──────┘     └─────────────┘
                           │
              ┌────────────┼────────────┐
              ▼            ▼            ▼
       ┌───────────┐ ┌───────────┐ ┌───────────┐
       │ Open-Meteo│ │ Gemini AI │ │ Telegram  │
       │    API    │ │    API    │ │   Bot     │
       └───────────┘ └───────────┘ └───────────┘
```

---

## 🐛 Troubleshooting

### WiFi Connection Issues
- Verify SSID and password
- Ensure ESP32 is within WiFi range
- Check if 2.4GHz network (ESP32 doesn't support 5GHz)

### Sensor Reading Failures
- Check wiring connections
- Verify DHT11 is powered (3.3V)
- Try a different GPIO pin

### Telegram Not Sending
- Verify bot token and chat ID
- Ensure you've started a conversation with the bot
- Check WiFi connectivity

### Gemini API Errors
- Verify API key is valid
- Check quota limits in Google AI Studio
- Ensure ESP32 has internet access

---

## 📈 Future Enhancements

- [ ] Add OLED display for local readings
- [ ] Multiple sensor support (BME280, BMP180)
- [ ] Data logging to SD card
- [ ] MQTT integration for Home Assistant
- [ ] Custom alert thresholds via web UI
- [ ] Weather forecast predictions
- [ ] Air quality monitoring (MQ-135)

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## 🙏 Acknowledgments

- [Open-Meteo](https://open-meteo.com/) - Free weather API
- [Google Gemini](https://ai.google.dev/) - AI capabilities
- [Telegram Bot API](https://core.telegram.org/bots/api) - Notifications
- [ArduinoJson](https://arduinojson.org/) - JSON parsing library

---

## 👨‍💻 Author

**Himanshu**

---

<p align="center">
  Made with ❤️ and an ESP32
</p>

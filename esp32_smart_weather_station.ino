/*
 * ═══════════════════════════════════════════════════════════════════════════
 * ESP32 SMART WEATHER STATION
 * ═══════════════════════════════════════════════════════════════════════════
 * 
 * Features:
 * - Indoor temperature & humidity monitoring (DHT11)
 * - Real-time outdoor weather data (Open-Meteo API - Noida)
 * - AI-powered health advisories (Google Gemini)
 * - Telegram bot notifications with smart alerts
 * - Responsive web dashboard
 * 
 * Author: Himanshu
 * License: MIT
 * ═══════════════════════════════════════════════════════════════════════════
 */

#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <time.h>

// ═══════════════════════════════════════════════════════════════════════════
// CONFIGURATION - UPDATE THESE VALUES
// ═══════════════════════════════════════════════════════════════════════════

// WiFi Credentials
const char* ssid = "Airtel_Airtel";
const char* password = "YOUR_WIFI_PASSWORD";

// Telegram Bot Configuration
const char* telegramBotToken = "xxxxx";
const char* telegramChatId = "xxxxx";

// Gemini AI Configuration
const char* geminiApiKey = "xxxxxx";

// Noida Coordinates (for outdoor weather)
const float NOIDA_LAT = 28.5355;
const float NOIDA_LON = 77.3910;

// DHT11 Sensor Configuration
#define DHTPIN 23
#define DHTTYPE DHT11

// Alert Thresholds
const float HUMIDITY_ALERT_THRESHOLD = 80.0;
const float HIGH_TEMP_THRESHOLD = 35.0;
const float LOW_TEMP_THRESHOLD = 15.0;

// ═══════════════════════════════════════════════════════════════════════════
// GLOBAL VARIABLES
// ═══════════════════════════════════════════════════════════════════════════

WebServer server(80);
DHT dht(DHTPIN, DHTTYPE);

// Sensor Data
float currentTemp = 0;
float currentHumidity = 0;
bool sensorValid = false;

// Outdoor Weather Data
float outdoorTemp = 0;
float outdoorHumidity = 0;
float outdoorFeelsLike = 0;
float outdoorWindSpeed = 0;
float outdoorUVIndex = 0;
int outdoorWeatherCode = 0;
String outdoorCondition = "Unknown";
unsigned long lastOutdoorFetch = 0;
const unsigned long OUTDOOR_FETCH_INTERVAL = 600000; // 10 minutes

// Alert State
bool alertActive = false;
unsigned long lastAlertTime = 0;
const unsigned long ALERT_COOLDOWN = 300000; // 5 minutes

// Location for historical weather
float userLat = 28.6139;
float userLon = 77.2090;

// ═══════════════════════════════════════════════════════════════════════════
// WEATHER CODE DESCRIPTIONS
// ═══════════════════════════════════════════════════════════════════════════

String getWeatherDescription(int code) {
  switch(code) {
    case 0: return "Clear sky ☀️";
    case 1: return "Mainly clear 🌤️";
    case 2: return "Partly cloudy ⛅";
    case 3: return "Overcast ☁️";
    case 45: case 48: return "Foggy 🌫️";
    case 51: case 53: case 55: return "Drizzle 🌧️";
    case 61: case 63: case 65: return "Rain 🌧️";
    case 66: case 67: return "Freezing rain 🌨️";
    case 71: case 73: case 75: return "Snowfall ❄️";
    case 77: return "Snow grains ❄️";
    case 80: case 81: case 82: return "Rain showers 🌦️";
    case 85: case 86: return "Snow showers 🌨️";
    case 95: return "Thunderstorm ⛈️";
    case 96: case 99: return "Thunderstorm with hail ⛈️";
    default: return "Unknown";
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// DHT SENSOR FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

bool readDHTSensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  
  if (isnan(h) || isnan(t)) {
    sensorValid = false;
    return false;
  }
  
  currentHumidity = h;
  currentTemp = t;
  sensorValid = true;
  return true;
}

// ═══════════════════════════════════════════════════════════════════════════
// OUTDOOR WEATHER FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

void fetchNoidaWeather() {
  HTTPClient http;
  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + 
               String(NOIDA_LAT, 4) + "&longitude=" + String(NOIDA_LON, 4) +
               "&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,wind_speed_10m,uv_index";
  
  http.begin(url);
  http.setTimeout(10000);
  
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      JsonObject current = doc["current"];
      outdoorTemp = current["temperature_2m"] | 0.0;
      outdoorHumidity = current["relative_humidity_2m"] | 0.0;
      outdoorFeelsLike = current["apparent_temperature"] | 0.0;
      outdoorWeatherCode = current["weather_code"] | 0;
      outdoorWindSpeed = current["wind_speed_10m"] | 0.0;
      outdoorUVIndex = current["uv_index"] | 0.0;
      outdoorCondition = getWeatherDescription(outdoorWeatherCode);
      
      Serial.println("✓ Outdoor weather updated");
    }
  }
  
  http.end();
}

// ═══════════════════════════════════════════════════════════════════════════
// GEMINI AI FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

String getGeminiAdvice(float indoorTemp, float indoorHum, float outTemp, float outHum, String condition) {
  WiFiClientSecure client;
  client.setInsecure();
  
  HTTPClient http;
  String url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=" + String(geminiApiKey);
  
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(15000);
  
  String prompt = "You are a health advisor for a smart weather station. Give a brief, practical health advisory (3-4 sentences max) based on these conditions:\n\n"
                  "INDOOR: Temperature: " + String(indoorTemp, 1) + "°C, Humidity: " + String(indoorHum, 1) + "%\n"
                  "OUTDOOR (Noida): Temperature: " + String(outTemp, 1) + "°C, Humidity: " + String(outHum, 1) + "%, Condition: " + condition + "\n\n"
                  "Focus on: clothing advice, hydration, activity recommendations, and any health precautions. Be creative and helpful. Use emojis.";
  
  JsonDocument requestDoc;
  JsonArray contents = requestDoc["contents"].to<JsonArray>();
  JsonObject content = contents.add<JsonObject>();
  JsonArray parts = content["parts"].to<JsonArray>();
  JsonObject part = parts.add<JsonObject>();
  part["text"] = prompt;
  
  String requestBody;
  serializeJson(requestDoc, requestBody);
  
  int httpCode = http.POST(requestBody);
  String advice = "Unable to get AI advice at this time.";
  
  if (httpCode == HTTP_CODE_OK) {
    String response = http.getString();
    JsonDocument responseDoc;
    DeserializationError error = deserializeJson(responseDoc, response);
    
    if (!error) {
      const char* text = responseDoc["candidates"][0]["content"]["parts"][0]["text"];
      if (text) {
        advice = String(text);
        advice.trim();
      }
    }
  }
  
  http.end();
  return advice;
}

// ═══════════════════════════════════════════════════════════════════════════
// TELEGRAM FUNCTIONS
// ═══════════════════════════════════════════════════════════════════════════

bool sendTelegramMessage(String message) {
  WiFiClientSecure client;
  client.setInsecure();
  
  HTTPClient http;
  String url = "https://api.telegram.org/bot" + String(telegramBotToken) + "/sendMessage";
  
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(10000);
  
  // Escape special characters for JSON
  message.replace("\\", "\\\\");
  message.replace("\"", "\\\"");
  message.replace("\n", "\\n");
  
  String payload = "{\"chat_id\":\"" + String(telegramChatId) + "\",\"text\":\"" + message + "\",\"parse_mode\":\"HTML\"}";
  
  int httpCode = http.POST(payload);
  bool success = (httpCode == HTTP_CODE_OK);
  
  if (success) {
    Serial.println("✓ Telegram message sent");
  } else {
    Serial.println("✗ Telegram error: " + String(httpCode));
  }
  
  http.end();
  return success;
}

void sendAlertWithAI(String alertType) {
  // Fetch fresh outdoor weather
  fetchNoidaWeather();
  
  // Get AI advice
  String aiAdvice = getGeminiAdvice(currentTemp, currentHumidity, outdoorTemp, outdoorHumidity, outdoorCondition);
  
  // Build message
  String message = "🚨 <b>SMART ALERT: " + alertType + "</b>\\n";
  message += "━━━━━━━━━━━━━━━━━━━━\\n\\n";
  
  message += "🏠 <b>INDOOR CONDITIONS</b>\\n";
  message += "├ 🌡️ Temperature: " + String(currentTemp, 1) + "°C\\n";
  message += "└ 💧 Humidity: " + String(currentHumidity, 1) + "%\\n\\n";
  
  message += "🌍 <b>OUTDOOR (NOIDA)</b>\\n";
  message += "├ 🌡️ Temperature: " + String(outdoorTemp, 1) + "°C\\n";
  message += "├ 💧 Humidity: " + String(outdoorHumidity, 1) + "%\\n";
  message += "├ 🌤️ Condition: " + outdoorCondition + "\\n";
  message += "├ 🌬️ Wind: " + String(outdoorWindSpeed, 1) + " km/h\\n";
  message += "└ ☀️ UV Index: " + String(outdoorUVIndex, 1) + "\\n\\n";
  
  message += "━━━━━━━━━━━━━━━━━━━━\\n";
  message += "🤖 <b>AI HEALTH ADVISORY</b>\\n";
  message += "━━━━━━━━━━━━━━━━━━━━\\n\\n";
  
  // Clean AI advice for Telegram
  aiAdvice.replace("\n", "\\n");
  aiAdvice.replace("\"", "'");
  message += aiAdvice + "\\n\\n";
  
  message += "━━━━━━━━━━━━━━━━━━━━\\n";
  message += "📍 <i>ESP32 Smart Weather Station</i>";
  
  sendTelegramMessage(message);
}

void checkAlerts() {
  if (!sensorValid) return;
  
  unsigned long now = millis();
  bool shouldAlert = false;
  String alertType = "";
  
  // Check humidity
  if (currentHumidity >= HUMIDITY_ALERT_THRESHOLD) {
    shouldAlert = true;
    alertType = "HIGH HUMIDITY";
  }
  // Check high temperature
  else if (currentTemp >= HIGH_TEMP_THRESHOLD) {
    shouldAlert = true;
    alertType = "HIGH TEMPERATURE";
  }
  // Check low temperature
  else if (currentTemp <= LOW_TEMP_THRESHOLD) {
    shouldAlert = true;
    alertType = "LOW TEMPERATURE";
  }
  
  if (shouldAlert && !alertActive && (now - lastAlertTime > ALERT_COOLDOWN)) {
    alertActive = true;
    lastAlertTime = now;
    sendAlertWithAI(alertType);
  }
  else if (!shouldAlert && alertActive) {
    alertActive = false;
    String message = "✅ <b>CONDITIONS NORMALIZED</b>\\n\\n";
    message += "🌡️ Temperature: " + String(currentTemp, 1) + "°C\\n";
    message += "💧 Humidity: " + String(currentHumidity, 1) + "%\\n\\n";
    message += "<i>All readings are now within normal range.</i>";
    sendTelegramMessage(message);
  }
}

// ═══════════════════════════════════════════════════════════════════════════
// WEB SERVER HANDLERS
// ═══════════════════════════════════════════════════════════════════════════

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Weather Station</title>
    <link href="https://fonts.googleapis.com/css2?family=Space+Grotesk:wght@300;400;500;600;700&family=JetBrains+Mono:wght@400;500&display=swap" rel="stylesheet">
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Space Grotesk', sans-serif;
            background: #000;
            color: #fff;
            min-height: 100vh;
            padding: 20px;
        }
        
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        
        header {
            text-align: center;
            padding: 40px 0;
            border-bottom: 1px solid #333;
            margin-bottom: 40px;
        }
        
        h1 {
            font-size: 2.5rem;
            font-weight: 300;
            letter-spacing: 8px;
            text-transform: uppercase;
            margin-bottom: 10px;
        }
        
        .subtitle {
            color: #666;
            font-size: 0.9rem;
            letter-spacing: 4px;
        }
        
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(350px, 1fr));
            gap: 30px;
            margin-bottom: 30px;
        }
        
        .card {
            background: #111;
            border: 1px solid #222;
            padding: 30px;
        }
        
        .card-title {
            font-size: 0.75rem;
            letter-spacing: 3px;
            text-transform: uppercase;
            color: #666;
            margin-bottom: 25px;
            padding-bottom: 15px;
            border-bottom: 1px solid #222;
        }
        
        .reading {
            display: flex;
            justify-content: space-between;
            align-items: baseline;
            padding: 15px 0;
            border-bottom: 1px solid #1a1a1a;
        }
        
        .reading:last-child {
            border-bottom: none;
        }
        
        .reading-label {
            color: #888;
            font-size: 0.9rem;
        }
        
        .reading-value {
            font-family: 'JetBrains Mono', monospace;
            font-size: 1.8rem;
            font-weight: 500;
        }
        
        .reading-unit {
            color: #666;
            font-size: 1rem;
            margin-left: 5px;
        }
        
        .status {
            display: inline-block;
            padding: 4px 12px;
            font-size: 0.7rem;
            letter-spacing: 2px;
            text-transform: uppercase;
            border: 1px solid;
        }
        
        .status.normal {
            color: #4a4a4a;
            border-color: #333;
        }
        
        .status.alert {
            color: #fff;
            border-color: #fff;
            background: #fff;
            color: #000;
        }
        
        .ai-section {
            background: #111;
            border: 1px solid #222;
            padding: 30px;
            margin-bottom: 30px;
        }
        
        .ai-content {
            font-size: 1rem;
            line-height: 1.8;
            color: #ccc;
            min-height: 100px;
        }
        
        .btn {
            background: #fff;
            color: #000;
            border: none;
            padding: 12px 30px;
            font-family: 'Space Grotesk', sans-serif;
            font-size: 0.8rem;
            letter-spacing: 2px;
            text-transform: uppercase;
            cursor: pointer;
            transition: all 0.3s ease;
        }
        
        .btn:hover {
            background: #ccc;
        }
        
        .btn:disabled {
            background: #333;
            color: #666;
            cursor: not-allowed;
        }
        
        .btn-outline {
            background: transparent;
            color: #fff;
            border: 1px solid #444;
        }
        
        .btn-outline:hover {
            background: #fff;
            color: #000;
        }
        
        .button-group {
            display: flex;
            gap: 15px;
            margin-top: 20px;
            flex-wrap: wrap;
        }
        
        .weather-condition {
            font-size: 1.2rem;
            margin-top: 10px;
            color: #888;
        }
        
        .outdoor-grid {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 20px;
        }
        
        .outdoor-item {
            padding: 15px;
            background: #0a0a0a;
        }
        
        .outdoor-label {
            font-size: 0.7rem;
            color: #666;
            letter-spacing: 2px;
            text-transform: uppercase;
            margin-bottom: 8px;
        }
        
        .outdoor-value {
            font-family: 'JetBrains Mono', monospace;
            font-size: 1.3rem;
        }
        
        footer {
            text-align: center;
            padding: 40px 0;
            color: #333;
            font-size: 0.8rem;
            letter-spacing: 2px;
        }
        
        @media (max-width: 768px) {
            h1 { font-size: 1.5rem; letter-spacing: 4px; }
            .grid { grid-template-columns: 1fr; }
            .outdoor-grid { grid-template-columns: 1fr; }
            .button-group { flex-direction: column; }
            .btn { width: 100%; }
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>Weather Station</h1>
            <p class="subtitle">ESP32 • DHT11 • Gemini AI</p>
        </header>
        
        <div class="grid">
            <div class="card">
                <div class="card-title">Indoor Sensor</div>
                <div class="reading">
                    <span class="reading-label">Temperature</span>
                    <span><span class="reading-value" id="temp">--</span><span class="reading-unit">°C</span></span>
                </div>
                <div class="reading">
                    <span class="reading-label">Humidity</span>
                    <span><span class="reading-value" id="humidity">--</span><span class="reading-unit">%</span></span>
                </div>
                <div class="reading">
                    <span class="reading-label">Alert Status</span>
                    <span class="status normal" id="alertStatus">Normal</span>
                </div>
            </div>
            
            <div class="card">
                <div class="card-title">Outdoor • Noida</div>
                <div class="weather-condition" id="condition">Loading...</div>
                <div class="outdoor-grid" style="margin-top: 20px;">
                    <div class="outdoor-item">
                        <div class="outdoor-label">Temperature</div>
                        <div class="outdoor-value"><span id="outTemp">--</span>°C</div>
                    </div>
                    <div class="outdoor-item">
                        <div class="outdoor-label">Feels Like</div>
                        <div class="outdoor-value"><span id="outFeels">--</span>°C</div>
                    </div>
                    <div class="outdoor-item">
                        <div class="outdoor-label">Humidity</div>
                        <div class="outdoor-value"><span id="outHumidity">--</span>%</div>
                    </div>
                    <div class="outdoor-item">
                        <div class="outdoor-label">Wind</div>
                        <div class="outdoor-value"><span id="outWind">--</span> km/h</div>
                    </div>
                    <div class="outdoor-item">
                        <div class="outdoor-label">UV Index</div>
                        <div class="outdoor-value" id="outUV">--</div>
                    </div>
                </div>
            </div>
        </div>
        
        <div class="ai-section">
            <div class="card-title">AI Health Advisory</div>
            <div class="ai-content" id="aiAdvice">
                Click "Get AI Advice" for personalized health recommendations based on current indoor and outdoor conditions.
            </div>
            <div class="button-group">
                <button class="btn" onclick="getAIAdvice()">Get AI Advice</button>
                <button class="btn btn-outline" onclick="testAlert()">Test Telegram Alert</button>
            </div>
        </div>
        
        <footer>
            UPTIME: <span id="uptime">0</span>s • Auto-refresh: 5s
        </footer>
    </div>
    
    <script>
        function updateReadings() {
            fetch('/readings')
                .then(r => r.json())
                .then(data => {
                    if (data.valid) {
                        document.getElementById('temp').textContent = data.temperature.toFixed(1);
                        document.getElementById('humidity').textContent = data.humidity.toFixed(1);
                    }
                    document.getElementById('uptime').textContent = data.uptime;
                    
                    const status = document.getElementById('alertStatus');
                    if (data.alertActive) {
                        status.textContent = 'ALERT';
                        status.className = 'status alert';
                    } else {
                        status.textContent = 'Normal';
                        status.className = 'status normal';
                    }
                });
        }
        
        function updateOutdoor() {
            fetch('/outdoor')
                .then(r => r.json())
                .then(data => {
                    document.getElementById('outTemp').textContent = data.temperature.toFixed(1);
                    document.getElementById('outFeels').textContent = data.feels_like.toFixed(1);
                    document.getElementById('outHumidity').textContent = data.humidity.toFixed(0);
                    document.getElementById('outWind').textContent = data.wind_speed.toFixed(1);
                    document.getElementById('outUV').textContent = data.uv_index.toFixed(1);
                    document.getElementById('condition').textContent = data.condition;
                });
        }
        
        function getAIAdvice() {
            const btn = event.target;
            btn.disabled = true;
            btn.textContent = 'Loading...';
            document.getElementById('aiAdvice').textContent = 'Consulting Gemini AI...';
            
            fetch('/readings')
                .then(r => r.json())
                .then(indoor => {
                    return fetch('/outdoor').then(r => r.json()).then(outdoor => ({indoor, outdoor}));
                })
                .then(data => {
                    // Request AI advice from ESP32
                    return fetch('/testAlert').then(r => r.text()).then(response => {
                        document.getElementById('aiAdvice').textContent = 'AI advice sent to Telegram! Check your phone for personalized health recommendations.';
                        btn.disabled = false;
                        btn.textContent = 'Get AI Advice';
                    });
                })
                .catch(err => {
                    document.getElementById('aiAdvice').textContent = 'Error getting AI advice. Please try again.';
                    btn.disabled = false;
                    btn.textContent = 'Get AI Advice';
                });
        }
        
        function testAlert() {
            const btn = event.target;
            btn.disabled = true;
            btn.textContent = 'Sending...';
            
            fetch('/testAlert')
                .then(r => r.text())
                .then(data => {
                    alert('Test alert sent to Telegram!');
                    btn.disabled = false;
                    btn.textContent = 'Test Telegram Alert';
                })
                .catch(err => {
                    alert('Error sending alert');
                    btn.disabled = false;
                    btn.textContent = 'Test Telegram Alert';
                });
        }
        
        // Initial load
        updateReadings();
        updateOutdoor();
        
        // Auto refresh
        setInterval(updateReadings, 5000);
        setInterval(updateOutdoor, 60000);
    </script>
</body>
</html>
)rawliteral";
  
  server.send(200, "text/html", html);
}

void handleReadings() {
  readDHTSensor();
  
  JsonDocument doc;
  doc["temperature"] = currentTemp;
  doc["humidity"] = currentHumidity;
  doc["valid"] = sensorValid;
  doc["uptime"] = millis() / 1000;
  doc["alertActive"] = alertActive;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleOutdoor() {
  JsonDocument doc;
  doc["temperature"] = outdoorTemp;
  doc["humidity"] = outdoorHumidity;
  doc["feels_like"] = outdoorFeelsLike;
  doc["wind_speed"] = outdoorWindSpeed;
  doc["uv_index"] = outdoorUVIndex;
  doc["weather_code"] = outdoorWeatherCode;
  doc["condition"] = outdoorCondition;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleTestAlert() {
  sendAlertWithAI("TEST ALERT");
  server.send(200, "text/plain", "Alert sent!");
}

void handleSetLocation() {
  if (server.hasArg("lat") && server.hasArg("lon")) {
    userLat = server.arg("lat").toFloat();
    userLon = server.arg("lon").toFloat();
    server.send(200, "text/plain", "Location updated");
  } else {
    server.send(400, "text/plain", "Missing lat/lon parameters");
  }
}

void handleWeatherAPI() {
  HTTPClient http;
  
  // Get dates for last 14 days
  time_t now;
  time(&now);
  struct tm* timeinfo = localtime(&now);
  
  char endDate[11];
  char startDate[11];
  strftime(endDate, 11, "%Y-%m-%d", timeinfo);
  
  time_t twoWeeksAgo = now - (14 * 24 * 60 * 60);
  struct tm* startTimeinfo = localtime(&twoWeeksAgo);
  strftime(startDate, 11, "%Y-%m-%d", startTimeinfo);
  
  String url = "https://archive-api.open-meteo.com/v1/archive?latitude=" + 
               String(userLat, 4) + "&longitude=" + String(userLon, 4) +
               "&start_date=" + String(startDate) + "&end_date=" + String(endDate) +
               "&daily=temperature_2m_max,temperature_2m_min,precipitation_sum";
  
  http.begin(url);
  http.setTimeout(15000);
  
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    server.send(200, "application/json", http.getString());
  } else {
    server.send(500, "application/json", "{\"error\":\"Failed to fetch weather data\"}");
  }
  
  http.end();
}

// ═══════════════════════════════════════════════════════════════════════════
// SETUP & LOOP
// ═══════════════════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(115200);
  Serial.println("\n");
  Serial.println("═══════════════════════════════════════════════════════");
  Serial.println("       ESP32 SMART WEATHER STATION");
  Serial.println("═══════════════════════════════════════════════════════");
  
  // Initialize DHT sensor
  dht.begin();
  Serial.println("✓ DHT11 sensor initialized");
  
  // Connect to WiFi
  Serial.print("→ Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi connected!");
    Serial.print("  IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n✗ WiFi connection failed!");
  }
  
  // Configure time
  configTime(19800, 0, "pool.ntp.org", "time.nist.gov"); // IST offset
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/readings", handleReadings);
  server.on("/outdoor", handleOutdoor);
  server.on("/api", handleWeatherAPI);
  server.on("/setLocation", HTTP_POST, handleSetLocation);
  server.on("/testAlert", handleTestAlert);
  
  server.begin();
  Serial.println("✓ Web server started");
  
  // Fetch initial outdoor weather
  fetchNoidaWeather();
  
  // Read initial sensor data
  readDHTSensor();
  
  // Send startup notification
  String startupMsg = "🚀 <b>WEATHER STATION ONLINE</b>\\n\\n";
  startupMsg += "📍 IP: " + WiFi.localIP().toString() + "\\n";
  startupMsg += "🌡️ Indoor: " + String(currentTemp, 1) + "°C\\n";
  startupMsg += "💧 Humidity: " + String(currentHumidity, 1) + "%\\n\\n";
  startupMsg += "<i>Monitoring started. Alerts enabled.</i>";
  sendTelegramMessage(startupMsg);
  
  Serial.println("═══════════════════════════════════════════════════════");
  Serial.println("  Open http://" + WiFi.localIP().toString() + " in browser");
  Serial.println("═══════════════════════════════════════════════════════\n");
}

void loop() {
  server.handleClient();
  
  // Read sensor every 5 seconds
  static unsigned long lastSensorRead = 0;
  if (millis() - lastSensorRead >= 5000) {
    readDHTSensor();
    lastSensorRead = millis();
    
    // Check for alerts
    checkAlerts();
  }
  
  // Fetch outdoor weather periodically
  if (millis() - lastOutdoorFetch >= OUTDOOR_FETCH_INTERVAL) {
    fetchNoidaWeather();
    lastOutdoorFetch = millis();
  }
}


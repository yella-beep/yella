
//------------------------------------------------Libraries used--------------------------------------------------------------------------------
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <DHT.h>
//-------------------------------pins used for devices----------------------------------------------------------------------------------------------
#define doorPin 13       // GPIO for the servo (door control)
#define relayPin 12      // GPIO for the relay (bulb control)
#define fanPin 14        // GPIO for the fan control
#define DHTPin 15        // GPIO for the DHT sensor
#define DHTTYPE DHT11    // DHT sensor type
//-------------------------------------ssid,password created by ESP32----------------------------------------------------------------------------------
const char* ssid = "ESPDevice";
const char* password = "12345678";

DHT dht(DHTPin, DHTTYPE);
Servo myservo;
WebServer server(80);

float thresholdTemp = 35.0; // Default threshold temperature
bool manualFanControl = false;
bool fanState = false; // Fan is initially off
bool manualOverrideActive = false; // Track if manual control is overriding automatic control

String doorState = "Locked";
String BULBstate = "OFF";
//------------------------------------------------------web page---------------------------------------------------------------------------------------
// HTML page with bulb, door lock, and fan control
const char web_page[] PROGMEM = R"=====(

<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Smart Home Dashboard</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background: linear-gradient(135deg, #74ebd5, #acb6e5);
      color: #333;
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 20px;
    }
    h1 {
      font-size: 2.5em;
      color: #274a7e;
      margin-bottom: 20px;
    }
    .dashboard {
      display: flex;
      flex-wrap: wrap;
      gap: 20px;
      justify-content: center;
    }
    .card {
      background-color: white;
      border-radius: 15px;
      box-shadow: 0px 8px 16px rgba(0, 0, 0, 0.2);
      padding: 20px;
      width: 240px;
      text-align: center;
      transition: transform 0.2s ease, box-shadow 0.2s ease;
    }
    .card:hover {
      transform: translateY(-5px);
      box-shadow: 0px 12px 24px rgba(0, 0, 0, 0.3);
    }
    .card h2 {
      font-size: 1.4em;
      margin-bottom: 10px;
      color: #333;
    }
    .icon {
      font-size: 60px;
      margin: 10px 0;
      transition: color 0.3s ease;
    }
    .bulb-icon {
      color: #ccc;
    }
    .bulb-on .bulb-icon {
      color: yellow;
      text-shadow: 0 0 40px yellow, 0 0 60px yellow, 0 0 80px yellow;
    }
    .lock-icon {
      color: #28a745;
    }
    .lock-locked .lock-icon {
      color: red;
      text-shadow: 0 0 10px red;
    }
    .temperature, .humidity {
      font-size: 1.2em;
      font-weight: bold;
      color: #274a7e;
    }
    .button {
      background-color: #4CAF50;
      border: none;
      padding: 10px 20px;
      margin-top: 10px;
      font-size: 1em;
      color: white;
      border-radius: 5px;
      cursor: pointer;
      transition: background-color 0.3s ease;
    }
    .button:hover {
      background-color: #45a049;
    }
    .switch {
      position: relative;
      display: inline-block;
      width: 60px;
      height: 30px;
    }
    .switch input {
      display: none;
    }
    .slider {
      position: absolute;
      cursor: pointer;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      background-color: #ccc;
      transition: 0.4s;
      border-radius: 30px;
    }
    .slider:before {
      position: absolute;
      content: "";
      height: 22px;
      width: 22px;
      left: 4px;
      bottom: 4px;
      background-color: white;
      transition: 0.4s;
      border-radius: 50%;
    }
    input:checked + .slider {
      background-color: #4CAF50;
    }
    input:checked + .slider:before {
      transform: translateX(28px);
    }
  </style>
</head>
<body>

  <h1>Smart Home Dashboard</h1>

  <div class="dashboard">
    <!-- Smart Bulb Control -->
    <div class="card" id="bulbCard">
      <h2>Smart Bulb</h2>
      <div class="icon bulb-icon" id="bulbIcon">💡</div>
      <label class="switch">
        <input type="checkbox" id="bulbSwitch" onclick="toggleBulb()">
        <span class="slider"></span>
      </label>
      <div id="bulbStatus">OFF</div>
    </div>

    <!-- Door Lock Control -->
    <div class="card lock-locked" id="lockCard">
      <h2>Door Lock</h2>
      <div class="icon lock-icon">🔒</div>
      <label class="switch">
        <input type="checkbox" id="lockSwitch" onclick="toggleLock()">
        <span class="slider"></span>
      </label>
      <div id="lockStatus">LOCKED</div>
    </div>

    <!-- Climate Control -->
    <div class="card">
      <h2>Climate Control</h2>
      <div class="temperature" id="tempValue">22.3°C</div>
      <div class="humidity" id="humidityValue">Humidity: 46%</div>
      <input type="number" id="tempThreshold" placeholder="Set Temperature" style="margin-top: 10px; padding: 5px; width: 80%;">
      <button class="button" onclick="setThreshold()">Set Threshold</button>
      <label class="switch" style="margin-top: 10px;">
        <input type="checkbox" id="fanSwitch" onclick="toggleFan()">
        <span class="slider"></span>
      </label>
      <div>Manual Fan Control</div>
    </div>
  </div>

  <script>
    function toggleBulb() {
      var bulbSwitch = document.getElementById("bulbSwitch");
      var bulbIcon = document.getElementById("bulbIcon");
      var statusText = document.getElementById("bulbStatus");
      sendData('BULB', bulbSwitch.checked ? 1 : 0);
      bulbIcon.style.color = bulbSwitch.checked ? "yellow" : "#ccc";
      statusText.textContent = bulbSwitch.checked ? "ON" : "OFF";
    }

    function toggleLock() {
      var lockSwitch = document.getElementById("lockSwitch");
      var statusText = document.getElementById("lockStatus");
      sendData('SER1', lockSwitch.checked ? 3 : 4);
      statusText.textContent = lockSwitch.checked ? "UNLOCKED" : "LOCKED";
    }

    function setThreshold() {
      var threshold = document.getElementById("tempThreshold").value;
      if (threshold) {
        sendData("THRESHOLD", threshold);
        alert("Temperature threshold set to " + threshold + "°C");
      } else {
        alert("Please enter a valid temperature threshold.");
      }
    }

    function toggleFan() {
      var fanSwitch = document.getElementById("fanSwitch");
      sendData('FAN', fanSwitch.checked ? 1 : 0);  // Send the fan state to the server
    }

    setInterval(function() {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          var data = JSON.parse(this.responseText);
          document.getElementById("tempValue").textContent = data.temperature + "°C";
          document.getElementById("humidityValue").textContent = "Humidity: " + data.humidity + "%";
        }
      };
      xhttp.open("GET", "/temperature", true);
      xhttp.send();
    }, 2500);

    function sendData(device, status) {
      var xhttp = new XMLHttpRequest();
      xhttp.open("GET", "control?device=" + device + "&status=" + status, true);
      xhttp.send();
    }
  </script>

</body>
</html>
)=====";

void handleRoot() {
  server.send(200, "text/html", web_page);
}
//---------------------------------------function used to controll devices------------------------------------------------------------------------------
void controlDevice() {
  String device = server.arg("device");
  String status = server.arg("status");

  if (device == "BULB") {
    digitalWrite(relayPin, status == "1" ? LOW : HIGH);
    BULBstate = (status == "1" ? "ON" : "OFF");
  } 
  else if (device == "SER1") {
    myservo.write(status == "3" ? 0 : 130);
    doorState = (status == "3" ? "Unlocked" : "Locked");
  } 
  else if (device == "THRESHOLD") {
    thresholdTemp = status.toFloat();
  } 
  else if (device == "FAN") {
    manualFanControl = (status == "1");
    manualOverrideActive = manualFanControl;
    fanState = manualFanControl;
    digitalWrite(fanPin, fanState ? LOW : HIGH);
  }

  server.send(200, "text/plain", "OK");
}
//--------------------------------calculate temperature DHT sensor------------------------------------------------------------------------------------------
void handleTemperature() {
  float temp = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Automatic control only if manual override is inactive
  if (!manualFanControl && !manualOverrideActive) {
    if (temp > thresholdTemp) {
      fanState = true;
      digitalWrite(fanPin, LOW); // Turn on fan
    } else {
      fanState = false;
      digitalWrite(fanPin, HIGH); // Turn off fan
    }
  }

  // Manual control handling
  if (manualFanControl) {
    digitalWrite(fanPin, fanState ? LOW : HIGH);
  }

  String jsonData = "{\"temperature\":" + String(temp) + ",\"humidity\":" + String(humidity) + ",\"fanState\":\"" + (fanState ? "ON" : "OFF") + "\"}";
  server.send(200, "application/json", jsonData);
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  myservo.attach(doorPin);
  myservo.write(130);
  pinMode(relayPin, OUTPUT);
  pinMode(fanPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
  digitalWrite(fanPin, HIGH);

  WiFi.softAP(ssid, password);
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/control", controlDevice);
  server.on("/temperature", handleTemperature);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

#include <Arduino.h>
#include <WiFi.h>
#include <map>

#define WS_MAX_QUEUED_MESSAGES 5
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <ArduinoJson.h>


#include "light.hpp"
#include "cam.hpp"
#include "robo.hpp"


Light light;
Camera camera;
Robo robo;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

uint16_t client_count = 0;
uint32_t last_img = millis();


void onWebSocketEvent(AsyncWebSocket * server, AsyncWebSocketClient * client,
                      AwsEventType type, void * arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    if (server->count() > MAX_CLIENTS) {
      client->close();
      Serial.println("Client limit reached, closing connection.");
      return;
    }

    client_count = server->count();
    Serial.printf("Client connected: %u\n", client->id());

    ws.textAll(String("{\"client_count\":") + client_count + "}");
  } else if (type == WS_EVT_DISCONNECT) {
    client_count = server->count();
    Serial.printf("Client disconnected: %u\n", client->id());

    ws.textAll(String("{\"client_count\":") + client_count + "}");
  } else if (type == WS_EVT_DATA) {
    String msg = String((const char*)data, len);
    if (msg == "getframe") {
      camera_fb_t * fb = camera.getFrame();
      if (fb) {
        client->binary(fb->buf, fb->len);
        camera.returnFrame(fb);
      }
      return;
    }

    Serial.printf("Message from client: %s\n", msg.c_str());

    int currentX = robo.getPositionX();
    int currentY = robo.getPositionY();

    int newX = currentX;
    int newY = currentY;

    int steps = 10;

    // json commands
    if (msg.startsWith("{")) {
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, msg);
      if (!err && doc["x"].is<int>() && doc["y"].is<int>()) {
        int x = doc["x"];
        int y = doc["y"];
        // Validate limits
        if (x < MIN_POS_X || x > MAX_POS_X || y < MIN_POS_Y || y > MAX_POS_Y) {
          client->text("Position out of bounds");
          Serial.println("Position out of bounds");
          return;
        }
        robo.setPosition(x, y);
      } else {
        Serial.println("Invalid JSON command");
        return;
      }
    } else if (msg == "center") {
      newX = START_POS_X;
      newY = START_POS_Y;
    } else if (msg == "up") {
      newY = max(currentY - steps, MIN_POS_Y);
    } else if (msg == "down") {
      newY = min(currentY + steps, MAX_POS_Y);
    } else if (msg == "left") {
      newX = min(currentX + steps, MAX_POS_X);
    } else if (msg == "right") {
      newX = max(currentX - steps, MIN_POS_X);
    } else if (msg == "get_pos") {
      client->text(String("{\"x\":") + currentX + ",\"y\":" + currentY + "}");
      return;
    } else if (msg == "client_count") {
      client->text(String("{\"client_count\":") + client_count + "}");
      return;
    } else if (msg == "get_limits") {
      client->text(String("{\"min_x\":") + MIN_POS_X + ",\"max_x\":" + MAX_POS_X +
                  ",\"min_y\":" + MIN_POS_Y + ",\"max_y\":" + MAX_POS_Y + "}");
      return;
    } else if (msg == "light_on") {
      //light.setColor(0xFFFFFF); // White
      //light.setBrightness(32);
      Serial.println("Light on (deactivated for now)");
      return;
    } else if (msg == "light_off") {
      //light.clear();
      Serial.println("Light off (deactivated for now)");
      return;
    } else {
      Serial.println("Unknown command");
      return;
    }

    if (newX != currentX || newY != currentY) {
      robo.setPosition(newX, newY);
    }

    ws.textAll(String("{\"x\":") + newX + ",\"y\":" + newY + "}");
  }
}


void sendFrameToClients() {
  if (millis() - last_img < FRAME_INTERVAL || ws.count() == 0) {
    return; // Frame-Intervall einhalten
  }

  camera_fb_t * fb = camera.getFrame();
  if (fb) {
    ws.binaryAll(fb->buf, fb->len);
    camera.returnFrame(fb);
  } else {
    Serial.println("Failed to get frame from camera");
  }

  last_img = millis();
}


void setup() {
  Serial.begin(115200);
  //setCpuFrequencyMhz(240);

  light.init();
  light.setBrightness(10);
  light.setColor(0xFFFFFF); // Weiß
  light.show();

  Serial.println("Starting up...");

  if (!camera.init()) {
    Serial.println("Camera init failed");
    return;
  }

  robo.init();

  if (!camera.connectToWiFi()) {
    Serial.println("WiFi connection failed");
    return;
  }

  Serial.println("Connected to WiFi");
  Serial.println("IP address: " + WiFi.localIP().toString());

  WiFi.setSleep(false);

  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html",
      "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>"
      "<title>SUSCam</title>"
      "<style>"
      "body { background-color: #121212; color: #f0f0f0; font-family: sans-serif; text-align: center; padding: 20px; }"
      "img { border: 2px solid #555; border-radius: 8px; background: #000; width: 800px; height: 600px; }"
      ".controls { margin-top: 20px; }"
      "button { margin: 5px; padding: 10px 16px; font-size: 16px; border: none; border-radius: 6px; background-color: #333; color: #fff; cursor: pointer; }"
      "button:hover { background-color: #555; }"
      ".sliders { margin-top: 20px; }"
      ".slider-label { margin-right: 10px; }"
      ".client-count { position: absolute; top: 20px; right: 40px; background: #222; padding: 8px 16px; border-radius: 8px; font-size: 18px; opacity: 0.85; z-index: 10; }"
      "</style></head><body>"
      "<div class='client-count'>Clients: <span id='clientCount'>?</span></div>"
      "<h1>SUSCam</h1>"
      "<img id='cam' width='800' height='600'>"
      "<div class='controls'>"
      "<div><button onclick=\"send('up')\">⬆</button></div>"
      "<div>"
      "<button onclick=\"send('left')\">⬅</button>"
      "<button onclick=\"send('center')\">⏺</button>"
      "<button onclick=\"send('right')\">➡</button>"
      "</div><div><button onclick=\"send('down')\">⬇</button></div>"
      "</div>"
      "<div style='margin-top:20px;'>"
      "<button id='lightBtn' onclick=\"toggleLight()\">Licht AN</button>"
      "</div>"
      "<div class='sliders'>"
      "<span class='slider-label'>X:</span>"
      "<input type='range' min='0' max='180' value='90' id='sliderX' oninput='sliderUpdate()'>"
      "<span id='valX'>90</span>"
      "<br>"
      "<span class='slider-label'>Y:</span>"
      "<input type='range' min='0' max='180' value='90' id='sliderY' oninput='sliderUpdate()'>"
      "<span id='valY'>90</span>"
      "</div>"
      "<script>"
      "let ws = new WebSocket('ws://' + location.host + '/ws');"
      "ws.binaryType = 'blob';"
      "window.send = function(cmd) {"
      "  if (ws.readyState === 1) ws.send(cmd);"
      "};"
      "let lastX = 90, lastY = 90;"
      "let minX = 0, maxX = 180, minY = 0, maxY = 180;"
      "let lightOn = true;"
      "function toggleLight() {"
      "  if (ws.readyState !== 1) return;"
      "  if (lightOn) {"
      "    ws.send('light_off');"
      "    document.getElementById('lightBtn').textContent = 'Licht AN';"
      "    lightOn = false;"
      "  } else {"
      "    ws.send('light_on');"
      "    document.getElementById('lightBtn').textContent = 'Licht AUS';"
      "    lightOn = true;"
      "  }"
      "}"
      "ws.onmessage = function(e) {"
      "  if (typeof e.data === 'string') {"
      "    try {"
      "      let obj = JSON.parse(e.data);"
      "      if(obj.client_count !== undefined) {"
      "        document.getElementById('clientCount').textContent = obj.client_count;"
      "      }"
      "      if(obj.min_x !== undefined) {"
      "        minX = obj.min_x; maxX = obj.max_x; minY = obj.min_y; maxY = obj.max_y;"
      "        document.getElementById('sliderX').min = minX;"
      "        document.getElementById('sliderX').max = maxX;"
      "        document.getElementById('sliderY').min = minY;"
      "        document.getElementById('sliderY').max = maxY;"
      "      }"
      "      if(obj.x !== undefined && obj.y !== undefined) {"
      "        document.getElementById('sliderX').value = obj.x;"
      "        document.getElementById('sliderY').value = obj.y;"
      "        document.getElementById('valX').textContent = obj.x;"
      "        document.getElementById('valY').textContent = obj.y;"
      "        lastX = obj.x; lastY = obj.y;"
      "      }"
      "    } catch(err) { console.log('String message:', e.data); }"
      "  } else {"
      "    var url = URL.createObjectURL(e.data);"
      "    document.getElementById('cam').src = url;"
      "    setTimeout(function() { URL.revokeObjectURL(url); }, 100);"
      "  }"
      "};"
      "ws.onopen = function() {"
      "  if (ws.readyState === 1) {"
      "    ws.send('get_limits');"
      "    ws.send('get_pos');"
      "  }"
      "};"
      "window.onbeforeunload = function() {"
      "  if (ws && ws.readyState === 1) {"
      "    ws.close();"
      "  }"
      "};"
      "function sliderUpdate() {"
      "  let x = parseInt(document.getElementById('sliderX').value);"
      "  let y = parseInt(document.getElementById('sliderY').value);"
      "  document.getElementById('valX').textContent = x;"
      "  document.getElementById('valY').textContent = y;"
      "  if (ws.readyState === 1 && (x !== lastX || y !== lastY)) {"
      "    ws.send(JSON.stringify({x:x, y:y}));"
      "    lastX = x; lastY = y;"
      "  }"
      "}"
      "</script></body></html>"
    );
  });

  server.begin();
  Serial.println("Setup complete. WebSocket server running.");
}

void loop() {
  robo.loop();

  light.loop();

  ws.cleanupClients();

  sendFrameToClients();
}

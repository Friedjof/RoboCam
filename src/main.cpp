// main.cpp – WebSocket-gesteuerte ESP32-CAM mit verbessertem Stream und CSS-Design

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "cam.hpp"
#include "robo.hpp"
#include <map>

Camera camera;
Robo robo;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

TaskHandle_t streamTaskHandle = NULL;
bool clientsWantStream = false;
std::map<uint32_t, bool> clientStreamMap; // Client-ID -> Stream-Wunsch

void notifyClients(camera_fb_t* fb) {
  if (!fb) return;
  for (uint8_t i = 0; i < ws.count(); i++) {
    AsyncWebSocketClient *c = ws.client(i);
    if (c && c->canSend()) {
      c->binary(fb->buf, fb->len);
    }
  }
}

void streamTask(void *parameter) {
  static int frameCount = 0;
  static unsigned long lastPrint = 0;
  static unsigned long lastFrameTime = 0;
  const unsigned long frameInterval = 100; // 100ms = max 10 FPS

  while (true) {
    unsigned long currentTime = millis();
    bool anyReady = false;
    
    if (currentTime - lastFrameTime >= frameInterval) {
      for (uint8_t i = 0; i < ws.count(); i++) {
        AsyncWebSocketClient *c = ws.client(i);
        if (c && clientStreamMap[c->id()] && c->canSend()) {
          anyReady = true;
          break;
        }
      }

      if (clientsWantStream && anyReady) {
        camera_fb_t * fb = camera.getFrame();
        if (fb) {
          notifyClients(fb);
          camera.returnFrame(fb);
          frameCount++;
          lastFrameTime = currentTime;
        }
      }
    }
    
    if (currentTime - lastPrint > 1000) {
      Serial.printf("Streaming FPS: %d\n", frameCount);
      frameCount = 0;
      lastPrint = currentTime;
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void onWebSocketEvent(AsyncWebSocket * server, AsyncWebSocketClient * client,
                      AwsEventType type, void * arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("Client connected: %u\n", client->id());
    clientStreamMap[client->id()] = false;
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("Client disconnected: %u\n", client->id());
    clientStreamMap.erase(client->id());
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
    if (msg == "stream") {
      clientStreamMap[client->id()] = true;
      clientsWantStream = true;
      return;
    }

    Serial.printf("Message from client: %s\n", msg.c_str());

    int currentX = robo.getPositionX();
    int currentY = robo.getPositionY();

    int newX = currentX;
    int newY = currentY;

    int steps = 10;

    if (msg == "center") {
      newX = 90;
      newY = 45;
    } else if (msg == "up") {
      newY = min(currentY + steps, MAX_POS_Y);
    } else if (msg == "down") {
      newY = max(currentY - steps, MIN_POS_Y);
    } else if (msg == "left") {
      newX = max(currentX - steps, MIN_POS_X);
    } else if (msg == "right") {
      newX = min(currentX + steps, MAX_POS_X);
    }

    // Nur bewegen, wenn sich etwas geändert hat
    if (newX != currentX || newY != currentY) {
      robo.setPosition(newX, newY);
    }

    // Rückmeldung an Client senden
    String response = String("{\"x\":") + newX + ",\"y\":" + newY + "}";
    client->text(response);
  }
}


void setup() {
  Serial.begin(115200);
  setCpuFrequencyMhz(240);

  if (!camera.init()) {
    Serial.println("Camera init failed");
    return;
  }

  robo.init();

  if (!camera.connectToWiFi()) {
    Serial.println("WiFi connection failed");
    return;
  }

  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(200, "text/html",
      "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>"
      "<title>RoboCam</title>"
      "<style>"
      "body { background-color: #121212; color: #f0f0f0; font-family: sans-serif; text-align: center; padding: 20px; }"
      "img { border: 2px solid #555; border-radius: 8px; background: #000; }"
      ".controls { margin-top: 20px; }"
      "button { margin: 5px; padding: 10px 16px; font-size: 16px; border: none; border-radius: 6px; background-color: #333; color: #fff; cursor: pointer; }"
      "button:hover { background-color: #555; }"
      "</style></head><body>"
      "<h1>RoboCam</h1>"
      "<img id='cam' width='320' height='240'>"
      "<div class='controls'>"
      "<div><button onclick=\"send('up')\">⬆</button></div>"
      "<div>"
      "<button onclick=\"send('left')\">⬅</button>"
      "<button onclick=\"send('center')\">🔄</button>"
      "<button onclick=\"send('right')\">➡</button>"
      "</div><div><button onclick=\"send('down')\">⬇</button></div>"
      "</div><script>"
      "let ws = new WebSocket('ws://' + location.host + '/ws');"
      "ws.binaryType = 'blob';"
      "ws.onmessage = function(e) {"
      "  if (typeof e.data === 'string') {"
      "    let pos = JSON.parse(e.data);"
      "    console.log('Position:', pos.x, pos.y);"
      "  } else {"
      "    let url = URL.createObjectURL(e.data);"
      "    document.getElementById('cam').src = url;"
      "    setTimeout(function() { URL.revokeObjectURL(url); }, 100);"
      "  }"
      "};"
      "ws.onopen = function() {"
      "  setInterval(function() {"
      "    if (ws.readyState === 1) ws.send('getframe');"
      "  }, 100);"
      "};"
      "function send(cmd) { if (ws.readyState === 1) ws.send(cmd); }"
      "</script></body></html>"
    );
  });

  server.begin();
  xTaskCreatePinnedToCore(streamTask, "StreamTask", 8192, NULL, 1, &streamTaskHandle, 1);
  Serial.println("Setup complete. WebSocket server running.");
}

void loop() {
  delay(1000);
}

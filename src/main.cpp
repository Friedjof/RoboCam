#include "Arduino.h"
#include "cam.hpp"

// Instanz der Kamera-Klasse
Camera camera;

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(false);
    Serial.println("\nRoboCam starting up...");
    
    // Kamera initialisieren
    if (!camera.init()) {
        Serial.println("Camera initialization failed!");
        return;
    }
    Serial.println("Camera initialized successfully");
    
    // Mit WLAN verbinden
    if (!camera.connectToWiFi()) {
        Serial.println("WiFi connection failed!");
        return;
    }
    
    // Stream-Server starten
    camera.startStreamServer();
    
    // Stream-Info anzeigen
    camera.printStreamInfo();
}

void loop() {
    // Hauptschleife - nichts zu tun, alles läuft über Callbacks
    delay(10);
}
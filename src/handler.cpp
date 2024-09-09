#include <Arduino.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <Preferences.h>
#include "main.h"


extern WebServer server;




void initHANDLERS() {
    server.on("/", HTTP_GET, []() {
        if (SPIFFS.exists("/index.html")) {
            File file = SPIFFS.open("/index.html", "r");
            server.streamFile(file, "text/html");
            file.close();
        } else {
            server.send(404, "text/plain", "File not found");
        }
    });

    server.on("/settings/network.html", HTTP_GET, []() {
        if (SPIFFS.exists("/network.html")) {
            File file = SPIFFS.open("/network.html", "r");
            server.streamFile(file, "text/html");
            file.close();
        } else {
            server.send(404, "text/plain", "File not found");
        }
    });

    server.on("/scan_wifi", HTTP_GET, []() {
        int n = WiFi.scanNetworks();
        String json = "[";
        for (int i = 0; i < n; ++i) {
            if (i) json += ",";
            json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",";
            json += "\"rssi\":" + String(WiFi.RSSI(i)) + "}";
        }
        json += "]";
        server.send(200, "application/json", json);
    });

    server.on("/save_settings", HTTP_POST, []() {
        String ssid = server.arg("ssid");
        String password = server.arg("password");
        bool mqttEnabled = server.arg("mqttEnabled") == "true";
        String mqttServer = server.arg("mqttServer");
        int mqttPort = server.arg("mqttPort").toInt();
        String mqttUser = server.arg("mqttUser");
        String mqttPass = server.arg("mqttPass");

        // Verwende die globale Instanz von ShelfClock, um auf pref zuzugreifen
        shelfClock.saveWiFiSettings(ssid, password);
        // Speichern der MQTT-Daten
        shelfClock.saveMQTTSettings(mqttEnabled, mqttServer, mqttPort, mqttUser, mqttPass);

        server.send(200, "text/plain", "Settings saved. Restarting...");
        delay(3000);
        ESP.restart();
    });

    server.on("/led_control", HTTP_GET, []() {
        String state = server.arg("state");
        if (state == "ON") {
            shelfClock.setLEDState("ON");
            server.send(200, "text/plain", "LED eingeschaltet");
        } else if (state == "OFF") {
            shelfClock.setLEDState("OFF");
            server.send(200, "text/plain", "LED ausgeschaltet");
        } else {
            server.send(400, "text/plain", "Ungültiger Status");
        }
    });
}

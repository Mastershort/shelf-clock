#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ElegantOTA.h>
#include <WiFiManager.h> 
#include <FastLED.h>
#include "handler.h"
#include "main.h"

#include <ArduinoHA.h>
//#define NUM_LEDS 100
//#define DATA_PIN 4
ShelfClock shelfClock;

ShelfClock::ShelfClock() {
    // Konstruktorimplementierung
}

ShelfClock::~ShelfClock() {
    // Destruktorimplementierung
}
//  FUNCTION DECLARATIONS

//void shelfclock.initPREFERENCES(); // setup - initialize preferences and load values
void initWIFI();  
void initHANDLERS(); 
void initSERVER(); 
void configModeCallback(WiFiManager *myWiFiManager);
void saveConfigCallback(); 
void setupMQTT();
void reconnect();  
void setLEDState(String state) ;
void onMqttConnected();// setup - load all web handlers

Preferences    pref;
WebServer server( 80 );
WiFiClient espClient;
PubSubClient client(espClient);

byte mac[] = {0x00, 0x10, 0xFA, 0x6E, 0x38, 0x4A};

HADevice device(mac, sizeof(mac));
HAMqtt mqtt(espClient, device);
HAButton buttonA("myButtonA");
HAButton buttonB("myButtonB");
HALight *LedOnBoard = nullptr;
void onStateCommand(bool state, HALight *sender)
{
    if (sender == LedOnBoard)
    {
        if(state){
            digitalWrite(LED_BUILTIN, HIGH);
        }else{
           digitalWrite(LED_BUILTIN, LOW); 
        }
    }
    sender->setState(state);
}

void onButtonCommand(HAButton* sender)
{
    if (sender == &buttonA) {
        
    } else if (sender == &buttonB) {
        // button B was clicked, do your logic here
    }
}
// define variables ------------------------------
//#define LED_BUILTIN 2
//  wifi  ----------------------------------------------------------------------
bool      apSwitch = 1;        // ap: 0 = off, 1 = on
bool      apHidden = 0;        // ap: 0 = visible, 1 = hidden
String    apSSID , apPass;      // ap: ssid & password
IPAddress apIPLocal;           // ap: ip address
IPAddress apGateway;           // ap: gateway address
IPAddress apSubnet;            // ap: subnet mask
int       apIP1, apIP2, apIP3; // ap: ip address in blocks (apIP4 = 1)
bool      wifiSwitch = 0;      // wifi: 0 = off, 1 = on
String    wifiSSID, wifiPass;
String    mqttServer,mqttUser, mqttPass;
int       mqttPort;
bool      mqttEnabled = 0;
//CRGB leds[NUM_LEDS];
// void fill(CRGB color)
// {
//     for(int i = 0; i < NUM_LEDS; i++) {
//         leds[i] = color;
//     }
// }


void setup() {
    Serial.begin(115200);
       // FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS); 
    pinMode(LED_BUILTIN, OUTPUT);
    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("An error occurred while mounting SPIFFS");
        return;
    }
    mqtt.begin("10.0.10.1",1883,"mqttUser","mqttPassword");
    mqtt.onConnected(onMqttConnected);
   

    // WiFiManager Object
    WiFiManager wifiManager;
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    // Try to connect to the saved network, if it fails, start a config portal
    if (!wifiManager.autoConnect("ShelfEdgeClock", "12345678")) {
        Serial.println("Failed to connect and hit timeout");
        delay(3000);
        ESP.restart();
        
    }
  

    initHANDLERS();
    initSERVER();
    device.setName("Arduino");
    device.setSoftwareVersion("1.0.0");

    // optional properties
    buttonA.setIcon("mdi:fire");
    buttonA.setName("Click me A");
    buttonB.setIcon("mdi:home");
    buttonB.setName("Click me B");
    LedOnBoard = new HALight("onboardID", HALight::BrightnessFeature | HALight::RGBFeature);
    LedOnBoard->setIcon("mdi:fire");
    LedOnBoard->setName("OnBoardLed");
    LedOnBoard->onStateCommand(onStateCommand);

    // press callbacks
    buttonA.onCommand(onButtonCommand);
    buttonB.onCommand(onButtonCommand);

    

    Serial.println("HTTP server started");
}

void loop(void) {
 
  server.handleClient();
   if (!client.connected()) {
        // reconnect();
  }
//      fill(CRGB::Red);
//     FastLED.show();
//      delay(500);
//   // Now turn the LED off, then pause
//     fill(CRGB::Black);
//     FastLED.show();
    delay(500);
    client.loop();
    mqtt.loop();

}

void onMqttConnected()
{
    Serial.println("Mqtt Connected");
}       
// void initHANDLERS() {
//     server.on("/", HTTP_GET, []() {
//         if (SPIFFS.exists("/index.html")) {
//             File file = SPIFFS.open("/index.html", "r");
//             server.streamFile(file, "text/html");
//             file.close();
//         } else {
//             server.send(404, "text/plain", "File not found");
//         }
//     });

//     server.on("/settings/network.html", HTTP_GET, []() {
//         if (SPIFFS.exists("/network.html")) {
//             File file = SPIFFS.open("/network.html", "r");
//             server.streamFile(file, "text/html");
//             file.close();
//         } else {
//             server.send(404, "text/plain", "File not found");
//         }
//     });

//     server.on("/scan_wifi", HTTP_GET, []() {
//         int n = WiFi.scanNetworks();
//         String json = "[";
//         for (int i = 0; i < n; ++i) {
//             if (i) json += ",";
//             json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",";
//             json += "\"rssi\":" + String(WiFi.RSSI(i)) + "}";
//         }
//         json += "]";
//         server.send(200, "application/json", json);
//     });

//     server.on("/save_settings", HTTP_POST, []() {
//         String ssid = server.arg("ssid");
//         String password = server.arg("password");
//         bool mqttEnabled = server.arg("mqttEnabled") == "true";
//         String mqttServer = server.arg("mqttServer");
//         int mqttPort = server.arg("mqttPort").toInt();
//         String mqttUser = server.arg("mqttUser");
//         String mqttPass = server.arg("mqttPass");

//         // Speichern der WiFi-Daten in den Preferences
//         pref.putString("wifiSSID", ssid);
//         pref.putString("wifiPass", password);

//         // Speichern der MQTT-Daten in den Preferences
//         pref.putBool("mqttEnabled", mqttEnabled);
//         pref.putString("mqttServer", mqttServer);
//         pref.putInt("mqttPort", mqttPort);
//         pref.putString("mqttUser", mqttUser);
//         pref.putString("mqttPass", mqttPass);

//         server.send(200, "text/plain", "Settings saved. Restarting...");
//         delay(3000);
//         ESP.restart();
//     });
//     server.on("/led_control", HTTP_GET, []() {
//     String state = server.arg("state");
//     if (state == "ON") {
//         setLEDState("ON");
//         server.send(200, "text/plain", "LED eingeschaltet");
//     } else if (state == "OFF") {
//         setLEDState("OFF");
//         server.send(200, "text/plain", "LED ausgeschaltet");
//     } else {
//         server.send(400, "text/plain", "Ungültiger Status");
//     }
// });


    // Weitere Handler hier hinzufügen
//}

void ShelfClock::setLEDState(const String &state) {
    if (state == "ON") {
        digitalWrite(LED_BUILTIN, HIGH);
        LedOnBoard->setState(1);
        
    } else if (state == "OFF") {
        digitalWrite(LED_BUILTIN, LOW);
        LedOnBoard->setState(0);
    }
}

void initSERVER() {
  Serial.println( "- Webserver" );
   ElegantOTA.begin(&server); // Start ElegantOTA
  server.begin();

}
void ShelfClock::initPREFERENCES(){
  pref.begin( "shelfclock", false );

  pref.putInt( "apSwitch", 1 );
  apSwitch      = pref.getInt( "apSwitch", 1 );
  apHidden      = pref.getInt( "apHidden", 0 );
  apSSID        = pref.getString( "apSSID", "ShelfEdgeClock" );
  apPass        = pref.getString( "apPass", "12345678" );
  apIP1         = pref.getInt( "apIP1", 192 );
  apIP2         = pref.getInt( "apIP2", 168 );
  apIP3         = pref.getInt( "apIP3", 100 );
  wifiSwitch    = pref.getInt( "wifiSwitch", 0 );
  wifiSSID      = pref.getString( "wifiSSID", "ENTER_SSID" );
  wifiPass      = pref.getString( "wifiPass", "ENTER_PASSWORD" );
  mqttServer = pref.getString("mqttServer", "mqtt_broker_ip");
  mqttPort = pref.getInt("mqttPort", 1883);
  mqttUser = pref.getString("mqttUser", "user");
  mqttPass = pref.getString("mqttPass", "password");
 

}
void initWIFI() {
    Serial.println("- WiFi");

    if (apSwitch && wifiSwitch) {
        Serial.println("  ap & wifi");
        WiFi.mode(WIFI_AP_STA); // set mode: access point & wifi client
    } else if (apSwitch && !wifiSwitch) {
        Serial.println("  ap only");
        WiFi.mode(WIFI_AP); // set mode: access point only
    } else if (!apSwitch && wifiSwitch) {
        Serial.println("  wifi only");
        WiFi.mode(WIFI_STA); // set mode: wifi client only
    } else {
        Serial.println("  wifi off");
        WiFi.mode(WIFI_OFF); // set mode: turn off wifi
    }

    // Access point
    if (apSwitch) {
        apIPLocal = IPAddress(apIP1, apIP2, apIP3, 1);           // IP address
        apGateway = IPAddress(apIP1, apIP2, apIP3, 1);            // Gateway
        apSubnet = IPAddress(255, 255, 255, 0);                   // Subnet
        WiFi.softAPConfig(apIPLocal, apGateway, apSubnet);        // Config AP
        WiFi.softAP(apSSID.c_str(), apPass.c_str(), 1, apHidden); // Start AP
    }

    // WiFi client
    if (wifiSwitch) {
        WiFi.begin(wifiSSID.c_str(), wifiPass.c_str()); // Connect to WiFi
        for (int i = 0; i < 10; i++) {                  // Loop 10 times
            delay(200);                                 // Wait before each check
            if (WiFi.status() == WL_CONNECTED) {        // Check if connected
                break;                                  // If connected, leave the cycle
            }
        }
        if (WiFi.status() != WL_CONNECTED) { // If not connected
            WiFi.disconnect();               // Turn off WiFi
        }
    }
}
void setupMQTT() {
    if (!pref.getBool("mqttEnabled", false)) {
        Serial.println("MQTT is disabled, skipping setup.");
        return;
    }

    // mqttServer = pref.getString("mqttServer", "192.168.1.100");
    // mqttPort = pref.getInt("mqttPort", 1883);
    // mqttUser = pref.getString("mqttUser", "user");
    // mqttPass = pref.getString("mqttPass", "password");

    // client.setServer(mqttServer.c_str(), mqttPort);
    // client.setCallback([](char* topic, byte* payload, unsigned int length) {
    //     String message;
    //     for (int i = 0; i < length; i++) {
    //         message += (char)payload[i];
    //     }
    //     Serial.print("Message arrived [");
    //     Serial.print(topic);
    //     Serial.print("]: ");
    //     Serial.println(message);

    //     if (String(topic) == "home/led/onboard") {
    //         if (message == "ON") {
    //             digitalWrite(LED_BUILTIN, HIGH);
    //             client.publish("home/led/status", "ON");
    //         } else if (message == "OFF") {
    //             digitalWrite(LED_BUILTIN, LOW);
    //             client.publish("home/led/status", "OFF");
    //         }
    //     }
    // });

    // reconnect();
}

// void reconnect() {
//     if (!pref.getBool("mqttEnabled", false)) {
//         return; // If MQTT is disabled, do nothing
//     }

//     while (!client.connected()) {
//         Serial.print("Connecting to MQTT...");
//         if (client.connect("ESP32Client", mqttUser.c_str(), mqttPass.c_str())) {
//             Serial.println("connected");
//             client.subscribe("home/led/onboard");
//         } else {
//             Serial.print("failed, rc=");
//             Serial.print(client.state());
//             Serial.println(" try again in 5 seconds");
//             delay(5000);
//         }
//     }
// }

void ShelfClock::saveWiFiSettings(const String &ssid, const String &password) {
    pref.putString("wifiSSID", ssid);
    pref.putString("wifiPass", password);
}

void ShelfClock::saveMQTTSettings(bool mqttEnabled, const String &mqttServer, int mqttPort, const String &mqttUser, const String &mqttPass) {
    pref.putBool("mqttEnabled", mqttEnabled);
    pref.putString("mqttServer", mqttServer);
    pref.putInt("mqttPort", mqttPort);
    pref.putString("mqttUser", mqttUser);
    pref.putString("mqttPass", mqttPass);
}
void configModeCallback(WiFiManager *myWiFiManager) {
    Serial.println("Entered Configuration Mode");
    Serial.println(WiFi.softAPIP());
    Serial.println(myWiFiManager->getConfigPortalSSID());
}

void saveConfigCallback() {
    Serial.println("Configuration saved");
}


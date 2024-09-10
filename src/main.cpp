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
#include <time.h>


#define DATA_PIN 16
#define COLOR_ORDER GRB
#define LEDS_PER_SEGMENT 9 //LEDS PER SEGMENT
#define SEGMENTS_PER_NUMBER 7
#define NUM_DIGITS 7
#define LEDS_PER_DIGIT (LEDS_PER_SEGMENT * SEGMENTS_PER_NUMBER)
#define FAKE_NUM_LEDS (NUM_DIGITS * LEDS_PER_DIGIT)
#define LEDS_SEG 37
#define DIGITS_LEDS (SEGMENTS_PER_NUMBER * LEDS_PER_SEGMENT)
#define LEDS_NUMBERS (LEDS_SEG * LEDS_PER_SEGMENT)
#define SPOT_LED (NUM_DIGITS * 2)
#define NUM_LEDS (LEDS_NUMBERS + SPOT_LED)
CRGB LEDs[NUM_LEDS]; // Array für die LEDs
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
void onMqttConnected();
void initLED();
void initNTP(); 
void getNTP(); 
void displayClock();// setup - load all web handlers

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
#define digit0 seg(0), seg(1), seg(2), seg(3), seg(4), seg(5), seg(6)
#define fdigit1 seg(2), seg(7), seg(10), seg(15), seg(8), seg(3), seg(9)
#define digit2 seg(10), seg(11), seg(12), seg(13), seg(14), seg(15), seg(16)
#define fdigit3 seg(12), seg(17), seg(20), seg(25), seg(18), seg(13), seg(19)
#define digit4 seg(20), seg(21), seg(22), seg(23), seg(24), seg(25), seg(26)
#define fdigit5 seg(22), seg(27), seg(30), seg(35), seg(28), seg(23), seg(29)
#define digit6 seg(30), seg(31), seg(32), seg(33), seg(34), seg(35), seg(36)
#define seg(n) n*LEDS_PER_SEGMENT, n*LEDS_PER_SEGMENT+1, n*LEDS_PER_SEGMENT+2, n*LEDS_PER_SEGMENT+3, n*LEDS_PER_SEGMENT+4, n*LEDS_PER_SEGMENT+5, n*LEDS_PER_SEGMENT+6, n*LEDS_PER_SEGMENT+7, n*LEDS_PER_SEGMENT+8
#define nseg(n) n*LEDS_PER_SEGMENT+8, n*LEDS_PER_SEGMENT+7, n*LEDS_PER_SEGMENT+6, n*LEDS_PER_SEGMENT+5, n*LEDS_PER_SEGMENT+4, n*LEDS_PER_SEGMENT+3, n*LEDS_PER_SEGMENT+2, n*LEDS_PER_SEGMENT+1, n*LEDS_PER_SEGMENT

const char* host = "shelfclock";
 

//  Time  ----------------------------------------------------------------------
struct tm ntpTime;                      // structure to save time
time_t    now;                          // this is the epoch
uint32_t  currMillisCore1   = millis(); // curr time core1
uint32_t  prevTimeSecCore1  = 0;        // prev time core1 (1 sec)
int       prevTimeSecCore0  = 0;        // prev time core0 (1 sec)
int       prevTimeMinCore0  = 0;        // prev time core0 (1 min)
int       prevTimeHourCore0 = 0;        // prev time core0 (1 hour)
int       prevTimeDayCore0  = 0;        // prev time core0 (1 day)

TaskHandle_t taskCore0;

//  mode CLOCK  ----------------------------------------------------------------
String clkAddress  = "ch.pool.ntp.org";
String clkTimeZone = "CET-1CEST,M3.5.0,M10.5.0/3";
int    clkFormat   = 1; // 0 = AM/PM, 1 = 24h
int    clkColor    = 0; // 0 = 2 def, 1 = 2 random

int clkColorHR, clkColorHG, clkColorHB;
int clkColorMR, clkColorMG, clkColorMB;




const uint16_t FAKE_LEDs[FAKE_NUM_LEDS] = {digit0, fdigit1, digit2, fdigit3, digit4, fdigit5, digit6};
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



byte numbers[] = {
  0b0111111,   // 0
  0b0100001,   // 1
  0b1110110,   // 2
  0b1110011,   // 3
  0b1101001,   // 4
  0b1011011,   // 5
  0b1011111,   // 6
  0b0110001,   // 7
  0b1111111,   // 8
  0b1111011,   // 9
  0b0000000    // Leerzeichen
};
CRGB alternateColor = CRGB::Black;
void fill(CRGB color)
{
    for(int i = 0; i < NUM_LEDS; i++) {
        LEDs[i] = color;
    }
}



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
    initNTP(); 
    initLED();
   
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
void initLED(){
    FastLED.addLeds<WS2812B,DATA_PIN,COLOR_ORDER>(LEDs,NUM_LEDS).setCorrection(TypicalLEDStrip);

}


void loop(void) {
 unsigned long currMillisCore1 = millis();
  server.handleClient();
   if (!client.connected()) {
        // reconnect();
  }
  if ( ( currMillisCore1 - prevTimeSecCore1 ) >= 1000 ) { // inside here every second
    prevTimeSecCore1 = currMillisCore1;                   // update previous reference time
    time( &now );                                         // read the current time
    localtime_r( &now, &ntpTime );  
                        // update ntpTime with the current time
    
    
   displayClock();
    } 
    
    
  // Now turn the LED off, then pause
    FastLED.show();
    client.loop();
    mqtt.loop();

}
void displayNumber(uint16_t number, byte segment, CRGB color) {

    uint16_t startindex = 0;
    switch (segment) {
    case 0: startindex = 0; break;
    case 1: startindex = (LEDS_PER_DIGIT * 1); break;
    case 2: startindex = (LEDS_PER_DIGIT * 2); break;
    case 3: startindex = (LEDS_PER_DIGIT * 3); break;
    case 4: startindex = (LEDS_PER_DIGIT * 4); break;
    case 5: startindex = (LEDS_PER_DIGIT * 5); break;
    case 6: startindex = (LEDS_PER_DIGIT * 6); break;
    }
    for(byte i =0; i<SEGMENTS_PER_NUMBER; i++)

     for (byte j = 0; j < LEDS_PER_SEGMENT; j++) { 
        yield();
        LEDs[FAKE_LEDs[i * LEDS_PER_SEGMENT + j + startindex]] = 
        ((numbers[number] & (1 << i)) == (1 << i)) ? color : alternateColor;



     }


}
void displayClock(){
    int  hour = ntpTime.tm_hour; // value of hours
    int  mins = ntpTime.tm_min;  // value of minutes
    //CHSV clkHColor;              // color for hours
    //CHSV clkMColor;              // color for minutes

  // adjust hours based on time format
  if ( !clkFormat ) {
    if ( hour > 12 ) {  // if hours > 12 ...
      hour = hour - 12; // ... turn 13:mm to 01:mm PM
    }
    if ( hour < 1 ) {   // if hours = 00 ...
      hour = hour + 12; // ... turn 00:mm to 12:mm AM
    }
  }

  // build digits
  int h1 = floor( hour / 10 ); // build h1
  int h2 = hour % 10;          // build h2
  int m1 = floor( mins / 10 ); // build m1
  int m2 = mins % 10;          // build m2

  displayNumber( h1, 6, CRGB::Blue ); // show first digit
  displayNumber( h2, 4, CRGB::Blue ); // show second digit
  displayNumber( m1, 2, CRGB::Blue ); // show third digit
  displayNumber( m2, 0, CRGB::Blue ); // show fourth digit



};

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
        displayNumber(3, 0, CRGB::Red);
        FastLED.show();
        LedOnBoard->setState(1);
        
    } else if (state == "OFF") {
        digitalWrite(LED_BUILTIN, LOW);
        displayNumber(1, 0, CRGB::Blue);
        FastLED.show();
        LedOnBoard->setState(0);
    }
}
void initNTP() {
  Serial.println( "- NTP" );
  if ( WiFi.status() == WL_CONNECTED ) {
    // connect NTP (0 TZ offset)
    configTime( 0, 0, clkAddress.c_str() );
    // overwrite TZ
    setenv( "TZ", clkTimeZone.c_str(), 1 );
    // adjust the TZ
    tzset();

    if ( !getLocalTime( &ntpTime ) ) {
      ESP.restart();
    }
  }
}
void getNTP() {
  if ( WiFi.status() == WL_CONNECTED ) {
    getLocalTime( &ntpTime );
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


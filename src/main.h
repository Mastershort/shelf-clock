#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <Preferences.h>

class ShelfClock {
public:
    ShelfClock();
    ~ShelfClock();
    void setLEDState(const String &state);
    void initPREFERENCES();
    void saveWiFiSettings(const String &ssid, const String &password);
    void saveMQTTSettings(bool mqttEnabled, const String &mqttServer, int mqttPort, const String &mqttUser, const String &mqttPass);
private:
    Preferences pref;  // Deklaration des Preferences-Objekts
    int apSwitch;
    int apHidden;
    String apSSID;
    String apPass;
    int apIP1;
    int apIP2;
    int apIP3;
    int wifiSwitch;
    String wifiSSID;
    String wifiPass;
    String mqttServer;
    int mqttPort;
    String mqttUser;
    String mqttPass;
};
extern ShelfClock shelfClock;

#endif // MAIN_H

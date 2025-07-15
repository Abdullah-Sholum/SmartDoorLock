#ifndef EMERGENCY_RESET_HANDLER_H
#define EMERGENCY_RESET_HANDLER_H

#include <Arduino.h>
#include "Display.h"
#include "KeypadHandler.h"
#include "WiFiHandler.h"
#include "FingerprintSensor.h"
#include "RFIDHandler.h"

class EmergencyResetHandler {
  private:
    String buffer = "";
    const String resetCode = "12345";  // Bisa Anda ubah nanti

    KeypadHandler &keypad;
    Display &lcd;
    WiFiHandler &wifi;
    FingerprintSensor &fp;
    RFIDHandler &rfid;
    Preferences &prefs;

  public:
    EmergencyResetHandler(KeypadHandler &keypadRef, Display &lcdRef, WiFiHandler &wifiRef, FingerprintSensor &fpRef, RFIDHandler &rfidRef, Preferences &prefsRef)
      : keypad(keypadRef), lcd(lcdRef), wifi(wifiRef), fp(fpRef), rfid(rfidRef), prefs(prefsRef) {}

    void checkForReset() {
      char key = keypad.read();
      if (key >= '0' && key <= '9') {
        buffer += key;
        if (buffer.length() > resetCode.length()) {
          buffer = buffer.substring(buffer.length() - resetCode.length());  // Jaga panjang buffer
        }

        if (buffer == resetCode) {
          performReset();
        }
      }
    }

    void performReset() {
      lcd.clear();
      lcd.showMessage("Reset Sistem...", 0, 0);

      // Reset semua konfigurasi
      wifi.resetCredentials();
      rfid.clearAllData();
      fp.clearAllFingerprints();
      prefs.clear();

      delay(1500);
      lcd.clear();
      lcd.showMessage("Reset selesai!", 0, 0);
      delay(2000);
      ESP.restart();  // Restart sistem
    }
};

#endif

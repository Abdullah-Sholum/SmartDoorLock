#include <Arduino.h>
#include <Wire.h> 
#include "credentials.h"         
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <PCF8575.h>
#include <Adafruit_Fingerprint.h>
#include <WiFi.h>
#include <WiFiManager.h> 
#include <Preferences.h>      
#include <BlynkSimpleEsp32.h> 
#include <time.h>
// #define FINGERPRINT_LEDON           0x01
// #define FINGERPRINT_LEDOFF          0x02
// #define FINGERPRINT_LEDBREATHING    0x03
// #define FINGERPRINT_LEDRED          0x04
// #define FINGERPRINT_LEDGREEN        0x05
// #define FINGERPRINT_LEDBLUE         0x06
// #define FINGERPRINT_LEDYELLOW       0x07
// #define FINGERPRINT_LEDCYAN         0x08
// #define FINGERPRINT_LEDMAGENTA      0x09
// #define FINGERPRINT_LEDWHITE        0x0A

class Display {
  private:
    LiquidCrystal_I2C lcd;  // objek LCD sebagai anggota class

  public:
    // Constructor: inisialisasi alamat I2C dan ukuran LCD
    Display() : lcd(0x27, 20, 4) {}

    // Method untuk memulai LCD
    void begin() {
      lcd.init();
      lcd.backlight();
    }

    // Method untuk pengujian LCD
    void testLcd() {
      // Pengetesan backlight
      lcd.backlight();
      delay(500);
      lcd.noBacklight();
      delay(500);
      lcd.backlight();

      // Tes penulisan
      lcd.setCursor(20, 3); 
      lcd.print("Starting System..");
      delay(800);
      lcd.clear();

      // Countdown
      lcd.setCursor(8, 1);
      lcd.print("3");
      delay(800);
      lcd.clear();

      lcd.setCursor(8, 1);
      lcd.print("2");
      delay(800);
      lcd.clear();

      lcd.setCursor(8, 1);
      lcd.print("1");
      delay(800);
      lcd.clear();
    }

    // Method tambahan untuk menampilkan teks umum
    void showMessage(const String& message, uint8_t col, uint8_t row) {
      lcd.setCursor(col, row);
      lcd.print(message);
    }

    // Method untuk membersihkan layar
    void clear() {
      lcd.clear();
    }
};

class RelayHandler {
  private:
    const int RelayHandlerMain   = 26;
    const int RelayHandlerSecond = 27;

  public:
    // Constructor: set mode
    RelayHandler() {
      pinMode(RelayHandlerMain, OUTPUT);
      pinMode(RelayHandlerSecond, OUTPUT);
    }

    // Method untuk mengetes kedua RelayHandler
    void testRelayHandler() {
      digitalWrite(RelayHandlerMain, HIGH);
      digitalWrite(RelayHandlerSecond, HIGH);
      delay(500);
      
      digitalWrite(RelayHandlerMain, LOW);
      digitalWrite(RelayHandlerSecond, LOW);
      delay(500);

      digitalWrite(RelayHandlerMain, HIGH);
      digitalWrite(RelayHandlerSecond, HIGH);
      delay(500);

      digitalWrite(RelayHandlerMain, LOW);
      digitalWrite(RelayHandlerSecond, LOW);
      delay(500);

      digitalWrite(RelayHandlerMain, HIGH);
      digitalWrite(RelayHandlerSecond, HIGH);
    }

    // Method untuk mengaktifkan RelayHandler
    void activateMain() {
      digitalWrite(RelayHandlerMain, LOW);  // tergantung tipe RelayHandler, aktif bisa LOW
    }

    void activateSecond() {
      digitalWrite(RelayHandlerSecond, LOW);
    }

    // Method untuk menonaktifkan RelayHandler
    void deactivateMain() {
      digitalWrite(RelayHandlerMain, HIGH);
    }

    void deactivateSecond() {
      digitalWrite(RelayHandlerSecond, HIGH);
    }

    // Matikan kedua RelayHandler
    void deactivateAll() {
      digitalWrite(RelayHandlerMain, HIGH);
      digitalWrite(RelayHandlerSecond, HIGH);
    }

    // Aktifkan kedua RelayHandler
    void activateAll() {
      digitalWrite(RelayHandlerMain, LOW);
      digitalWrite(RelayHandlerSecond, LOW);
    }
};

class KeypadHandler {
  private:
    PCF8575 pcf;
    const byte ROWS = 4;
    const byte COLS = 3;

    char keys[4][3] = {
      {'1','2','3'},
      {'4','5','6'},
      {'7','8','9'},
      {'*','0','#'}
    };

    byte rowPins[4] = {0, 1, 2, 3};  // Baris = input
    byte colPins[3] = {6, 5, 4};     // Kolom = output

    char lastKey = '\0';

    Display &lcd; 

  public:
    // Constructor
    KeypadHandler(Display &displayRef) : pcf(0x24), lcd(displayRef) {}

    // Inisialisasi PCF dan I/O
    void begin() {
      while (!pcf.begin()) {
        lcd.clear();
        lcd.showMessage("PCF8575 tidak", 0, 0);
        lcd.showMessage("terdeteksi!", 0, 1);
        delay(2000);
      }

      for (byte c = 0; c < COLS; c++) {
        pcf.write(colPins[c], HIGH); // kolom idle HIGH
      }

      for (byte r = 0; r < ROWS; r++) {
        pcf.write(rowPins[r], HIGH); // baris input pull-up
      }
    }

    // Membaca key yang ditekan (return '\0' jika tidak ada)
    char read() {
      for (byte c = 0; c < COLS; c++) {
        pcf.write(colPins[c], LOW);  // aktifkan kolom

        for (byte r = 0; r < ROWS; r++) {
          if (pcf.read(rowPins[r]) == LOW) {
            lastKey = keys[r][c];
            delay(50);  // debounce
            while (pcf.read(rowPins[r]) == LOW);  // tunggu tombol dilepas
            pcf.write(colPins[c], HIGH);
            return lastKey;
          }
        }

        pcf.write(colPins[c], HIGH);
      }

      return '\0';
    }

    // Mengecek apakah tombol terakhir adalah karakter tertentu
    bool key(char c) {
      return lastKey == c;
    }

    // Reset nilai lastKey
    void clearLastKey() {
      lastKey = '\0';
    }

    String readPIN() {
      String pin = "";
      lcd.clear();  // jika tidak menggunakan LCD, hapus baris ini
      lcd.showMessage("Masukkan PIN:", 0, 0);

      while (pin.length() < 4) {
        char key = read();  // gunakan method internal read() untuk membaca keypad
        if (key >= '0' && key <= '9') {
          pin += key;

          // tampilkan karakter * sesuai jumlah digit
          String masked = "";
          for (int i = 0; i < pin.length(); i++) masked += "*";
          lcd.showMessage(masked, 0, 1);
        }
      }

      delay(500);  // jeda sebentar sebelum keluar
      lcd.clear(); // jika tidak menggunakan LCD, hapus baris ini
      return pin;
    }

    // Method testKeypad: tampilkan tombol yang ditekan di Display
    void testKeypad() {
      char key = read();
      if (key != '\0') {
        lcd.clear();
        lcd.showMessage("Tombol ditekan:", 0, 0);
        lcd.showMessage(String(key), 7, 1);
      }
    }
};

class RFIDHandler {
  private:
    const uint8_t SS_PIN  = 15;
    const uint8_t RST_PIN = 5;
    MFRC522 rfid;

    Preferences prefs;
    Display &lcd;
    KeypadHandler &keypad;

  public:
    RFIDHandler(Display &displayRef, KeypadHandler &keypadRef)
      : rfid(SS_PIN, RST_PIN), lcd(displayRef), keypad(keypadRef) {}

    void begin() {
      SPI.begin(18, 19, 23, SS_PIN);
      rfid.PCD_Init();
      prefs.begin("rfid_db", false);
    }

    // Membaca UID dari kartu dan mengembalikannya sebagai String uppercase
    String readUID() {
      if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return "";

      String uid = "";
      for (byte i = 0; i < rfid.uid.size; i++) {
        if (rfid.uid.uidByte[i] < 0x10) uid += "0";
        uid += String(rfid.uid.uidByte[i], HEX);
      }
      uid.toUpperCase();

      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      return uid;
    }

    // Mengecek apakah UID sudah terdaftar, mengembalikan index jika ya, 0 jika tidak
    int getRegisteredIDFromUID(const String &uid) {
      String uidUpper = uid;
      uidUpper.toUpperCase();
      for (int i = 1; i <= 50; i++) {
        if (prefs.getString(("uid" + String(i)).c_str(), "") == uidUpper) {
          return i;
        }
      }
      return 0;
    }

    // Diperuntukkan pemanggilan di AccessManager::rfidSecurity
    int checkUIDAndGetID(String &outUID) {
      outUID = readUID();
      if (outUID == "") return -1;
      return getRegisteredIDFromUID(outUID);
    }

    // Mengambil PIN tersimpan dari ID
    String getStoredPIN(int id) {
      return prefs.getString(("pin" + String(id)).c_str(), "");
    }

    // Mendaftarkan kartu dengan PIN
    void enrollWithPIN(int id) {
      String keyUID = "uid" + String(id);
      String keyPIN = "pin" + String(id);
      String keyID  = "id"  + String(id);

      if (prefs.isKey(keyUID.c_str()) || prefs.isKey(keyPIN.c_str()) || prefs.isKey(keyID.c_str())) {
        lcd.clear();
        lcd.showMessage("ID sudah dipakai!", 0, 0);
        delay(1500);
        lcd.clear();
        return;
      }

      lcd.clear();
      lcd.showMessage("Tempelkan kartu", 0, 0);
      delay(800);
      String uid = readUID();

      if (uid == "") {
        lcd.clear();
        lcd.showMessage("Gagal membaca", 0, 0);
        lcd.showMessage("kartu", 0, 1);
        delay(1500);
        lcd.clear();
        return;
      }

      if (getRegisteredIDFromUID(uid) > 0) {
        lcd.clear();
        lcd.showMessage("Kartu sudah", 0, 0);
        lcd.showMessage("terdaftar", 0, 1);
        delay(1500);
        lcd.clear();
        return;
      }

      lcd.clear();
      lcd.showMessage("Masukkan PIN:", 0, 0);
      String pin = "";
      while (pin.length() < 4) {
        char key = keypad.read();
        if (key >= '0' && key <= '9') {
          pin += key;
          String masked = "";
          for (int i = 0; i < pin.length(); i++) masked += "*";
          lcd.showMessage(masked, 0, 1);
        }
      }

      for (int i = 1; i <= 50; i++) {
        if (prefs.getString(("pin" + String(i)).c_str(), "") == pin) {
          lcd.clear();
          lcd.showMessage("PIN sudah", 0, 0);
          lcd.showMessage("terdaftar!", 0, 1);
          delay(1500);
          lcd.clear();
          return;
        }
      }

      prefs.putString(keyUID.c_str(), uid);
      prefs.putString(keyPIN.c_str(), pin);
      prefs.putInt(keyID.c_str(), id);

      lcd.clear();
      lcd.showMessage("Pendaftaran OK", 0, 0);
      lcd.showMessage("ID: " + String(id), 0, 1);
      delay(1500);
      lcd.clear();
    }

    // Menghapus semua data untuk ID tertentu
    void clearIndex(int index) {
      prefs.remove(("uid" + String(index)).c_str());
      prefs.remove(("pin" + String(index)).c_str());
      prefs.remove(("id"  + String(index)).c_str());
    }

    void setMethod(int index, const String &method) {
      Preferences methodPrefs;
      methodPrefs.begin("rfid_method", false);
      methodPrefs.putString(("m" + String(index)).c_str(), method);
      methodPrefs.end();
    }

    // Ambil metode autentikasi dari ID
    String getMethod(int index) {
      Preferences methodPrefs;
      methodPrefs.begin("rfid_method", true);
      String method = methodPrefs.getString(("m" + String(index)).c_str(), "none");
      methodPrefs.end();
      return method;
    }

    // Reset total semua UID & PIN
    void deleteAll() {
      for (int i = 1; i <= 50; i++) clearIndex(i);
    }

    void end() {
      prefs.end();
    }
};

class FingerprintSensor {
  private:
    HardwareSerial mySerial;
    Adafruit_Fingerprint finger;
    Display &lcd;  

  public:
    // Constructor
    FingerprintSensor(Display &displayRef)
      : mySerial(2), finger(&mySerial), lcd(displayRef) {}

    // Inisialisasi sensor fingerprint
    void begin() {
      mySerial.begin(57600, SERIAL_8N1, 16, 17);
      finger.begin(57600);          

      if (finger.verifyPassword()) {
        lcd.showMessage("Sensor Finger OK", 0, 0);
      } else {
        lcd.showMessage("Sensor Finger", 2, 0);
        lcd.showMessage("FAIL", 6, 1);
      }
      delay(1500);
      lcd.clear();
    }

    // Method simpan sidik jari
    void enrollFingerSecure(uint8_t id) {
      lcd.clear();
      lcd.showMessage("Daftar ID: " + String(id), 0, 0);
      delay(1000);

      // Cek apakah ID sudah dipakai
      if (finger.loadModel(id) == FINGERPRINT_OK) {
        lcd.showMessage("ID sudah dipakai", 0, 1);
        delay(2000);
        lcd.clear();
        return;
      }

      // Gambar pertama
      lcd.showMessage("Letakkan jari", 0, 1);
      finger.LEDcontrol(true);  // Nyalakan LED
      while (finger.getImage() != FINGERPRINT_OK);
      finger.LEDcontrol(false); // Matikan LED setelah gambar diambil
      if (finger.image2Tz(1) != FINGERPRINT_OK) {
        lcd.showMessage("Gagal gambar 1", 0, 1);
        delay(2000);
        lcd.clear();
        return;
      }

      lcd.showMessage("Angkat jari...", 0, 1);
      delay(2000);
      while (finger.getImage() != FINGERPRINT_NOFINGER);

      // Gambar kedua
      lcd.showMessage("Ulangi jari", 0, 1);
      while (finger.getImage() != FINGERPRINT_OK);
      if (finger.image2Tz(2) != FINGERPRINT_OK) {
        lcd.showMessage("Gagal gambar 2", 0, 1);
        delay(2000);
        lcd.clear();
        return;
      }

      // Cocokkan template
      if (finger.createModel() != FINGERPRINT_OK) {
        lcd.showMessage("Tidak cocok, ulangi", 0, 1);
        delay(2000);
        lcd.clear();
        return;
      }

      // Simpan model ke ID
      if (finger.storeModel(id) != FINGERPRINT_OK) {
        lcd.showMessage("Gagal simpan data", 0, 1);
        delay(2000);
        lcd.clear();
        return;
      }

      // Verifikasi ulang untuk memastikan jari sama
      lcd.showMessage("Verifikasi ulang...", 0, 1);
      delay(1500);

      // Minta jari kembali
      lcd.clear();
      lcd.showMessage("Tempel jari lagi", 0, 0);
      while (finger.getImage() != FINGERPRINT_OK);
      if (finger.image2Tz(1) != FINGERPRINT_OK) {
        lcd.showMessage("Gagal verifikasi", 0, 1);
        delay(2000);
        lcd.clear();
        return;
      }
      if (finger.fingerSearch() != FINGERPRINT_OK || finger.fingerID != id) {
        lcd.showMessage("Gagal cocok ulang", 0, 1);
        // Hapus data karena hasil verifikasi gagal
        finger.deleteModel(id);
        delay(2000);
        lcd.clear();
        return;
      }

      // Jika berhasil
      lcd.showMessage("Daftar Berhasil!", 0, 1);
      delay(2000);
      lcd.clear();
    }

    // Method pembacaan ID jika ada
    int readIDStatus() {
      uint8_t p = finger.getImage();
      if (p == FINGERPRINT_NOFINGER) return -2;
      if (p != FINGERPRINT_OK) return -1;

      if (finger.image2Tz() != FINGERPRINT_OK) return -1;
      if (finger.fingerSearch() != FINGERPRINT_OK) return -1;

      return finger.fingerID;  // ID ditemukan
    }

    // Method debugging: ambil 1 gambar jari
    void testCapture() {
      lcd.clear();
      lcd.showMessage("Letakkan jari...", 0, 0);
      delay(800);
      lcd.clear();

      int result = finger.getImage();
      if (result == FINGERPRINT_OK) {
        lcd.showMessage("Gambar diterima", 0, 1);
      } else if (result == FINGERPRINT_NOFINGER) {
        lcd.showMessage("Tidak ada jari", 0, 1);
      } else if (result == FINGERPRINT_IMAGEFAIL) {
        lcd.showMessage("Gagal ambil gambar", 0, 1);
      } else {
        lcd.showMessage("Error kode: " + String(result), 0, 1);
      }

      delay(2000);
      lcd.clear();
    }

    // Method debugging: tampilkan ID ke display
    void testMatch() {
      lcd.clear();
      lcd.showMessage("Tempelkan jari...", 0, 0);
      delay(500);
      int id = readIDStatus();
      if (id >= 0) {
        lcd.showMessage("ID ditemukan: " + String(id), 0, 1);
      } else {
        lcd.showMessage("ID tidak ditemukan", 0, 1);
      }

      delay(2000);
      lcd.clear();
    }

    // Method cek jumlah template
    void printTemplateCount() {
      finger.getTemplateCount();
      lcd.clear();
      lcd.showMessage("Template: " + String(finger.templateCount), 0, 0);
      delay(2000);
      lcd.clear();
    }

    // Method menghapus template berdasarkan index
    void deleteFingerprint(int index) {
      finger.deleteModel(index);  // pastikan finger adalah instance Adafruit_Fingerprint atau serupa
    }

    // Method menghapus seluruh template
    void deleteAll() {
      lcd.clear();
      lcd.showMessage("Menghapus semua...", 0, 0);

      if (finger.emptyDatabase() == FINGERPRINT_OK) {
        lcd.showMessage("Template dihapus", 0, 1);
      } else {
        lcd.showMessage("Gagal hapus data", 0, 1);
      }

      delay(2000);
      lcd.clear();
    }
};

class WiFiHandler { 
  private:
    WiFiManager wm;
    char blynkToken[34]; 
    WiFiManagerParameter custom_blynk;
    Preferences prefs;
    Display &lcd; 

  public:
    // Constructor
    WiFiHandler(Display &displayRef)
      : lcd(displayRef), prefs(),
        custom_blynk("blynk", "Blynk Token", loadSavedToken().c_str(), 33)
    {}

    // Memulai koneksi dan menampilkan portal jika perlu
    void begin(const char* apName = "ESP32_Config", const char* apPassword = "12345678") {
      // Tampilkan status awal ke LCD
      lcd.clear();
      lcd.showMessage("Mencoba koneksi", 0, 0);
      lcd.showMessage("WiFi...", 0, 1);

      // Tambahkan parameter Blynk ke form konfigurasi
      wm.addParameter(&custom_blynk);

      // Coba auto connect, jika gagal buka portal konfigurasi
      if (!wm.autoConnect(apName, apPassword)) {
        lcd.clear();
        lcd.showMessage("Koneksi Gagal!", 0, 0);
        lcd.showMessage("Restarting...", 0, 1);
        delay(3000);
        ESP.restart();
      }

      // Jika berhasil terkoneksi
      lcd.clear();
      lcd.showMessage("WiFi Terkoneksi!", 0, 0);
      // lcd.showMessage(WiFi.localIP().toString(), 0, 1);
      lcd.showMessage( WiFi.SSID(), 0, 1);
      delay(2000);
      lcd.clear();
      
      //  Simpan token ke preferences
      strncpy(blynkToken, custom_blynk.getValue(), sizeof(blynkToken));
      if (strlen(blynkToken) > 0) {
        prefs.begin("blynk", false);
        prefs.putString("token", blynkToken);
        prefs.end();
        Serial.println("Token Blynk disimpan: " + String(blynkToken));
      } else {
        Serial.println("Token kosong, tidak disimpan.");
      }
    }

    // Method untuk mendapatkan token Blynk dari preferences
    String loadSavedToken() {
      prefs.begin("blynk", true);
      String token = prefs.getString("token", "");
      prefs.end();
      return token;
    }

    // Memulai Blynk dengan token yang disimpan
    void beginBlynk() {
      String token = getBlynkToken();
      
      if (WiFi.status() == WL_CONNECTED && token != "") {
        Blynk.config(token.c_str());
        
        if (Blynk.connect(5000)) {  // timeout dalam 5 detik
          lcd.clear();
          lcd.showMessage("Blynk Terkoneksi!", 0, 0);
          delay(2000);
          lcd.clear();
        } else {
          lcd.clear();
          lcd.showMessage("Blynk Gagal!", 0, 0);
        }
      } else {
        lcd.clear();
        lcd.showMessage("WiFi/Token", 0, 0);
        lcd.showMessage("Kosong", 0, 1);
      }
    }

    // Ambil token dari preferences
    String getBlynkToken() {
      prefs.begin("blynk", true);  // read-only
      String token = prefs.getString("token", "");
      prefs.end();
      return token;
    }

    // Cek status koneksi WiFi
    bool isConnected() {
      return WiFi.status() == WL_CONNECTED;
      lcd.clear();
      lcd.showMessage("Status WiFi:", 0, 0);
      lcd.showMessage(WiFi.status() == WL_CONNECTED ? "Terkoneksi" : "Tidak Terkoneksi", 0, 1);
      delay(2000);
      lcd.clear();
    }

    // Menampilkan SSID yang terkoneksi
    void getSSID() {
      // return WiFi.SSID();
      lcd.clear();
      lcd.showMessage("SSID: ", 0, 0);
      lcd.showMessage( WiFi.SSID(), 0, 1);
      delay(2000);
      lcd.clear();
    }

    // Menampilkan IP saat ini
    void getIP() {
      // return WiFi.localIP().toString();
      lcd.clear();
      lcd.showMessage("IP: " + WiFi.localIP().toString(), 0, 0);
      delay(2000);
      lcd.clear();
    }

    // Reset semua pengaturan WiFi & token Blynk
    void resetSettings() {
      wm.resetSettings(); // reset kredensial WiFi

      // Hapus token dari preferences
      prefs.begin("blynk", false);
      prefs.clear(); // hapus semua key di namespace blynk
      prefs.end();

      lcd.clear();
      lcd.showMessage("Pengaturan WiFi", 0, 0);
      lcd.showMessage("direset, restart", 0, 1);
      delay(2000);
      lcd.clear();
      ESP.restart();
    }
};

class TimeHandler {
  private:
    const char* ntpServer = "pool.ntp.org";
    const long gmtOffset_sec = 7 * 3600;  // GMT+7 WIB
    const int daylightOffset_sec = 0;
    Display &lcd;

  public:
    // Constructor dengan referensi ke objek Display
    TimeHandler(Display &displayRef) : lcd(displayRef) {}

    // Inisialisasi waktu NTP
    void begin() {
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
      struct tm timeinfo;
      if (!getLocalTime(&timeinfo)) {
        lcd.clear();
        lcd.showMessage("Gagal sinkron", 0, 0);
        lcd.showMessage("waktu", 0, 1);
        delay(2000);
        lcd.clear();
      } else {
        lcd.clear();
        lcd.showMessage("Waktu sinkron", 0, 0);
        delay(1500);
        lcd.clear();
      }
    }

    // Menampilkan waktu dan tanggal ke LCD
    void showDateTime() {
      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        char timeStr[16];
        char dateStr[16];
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
        strftime(dateStr, sizeof(dateStr), "%d/%m/%Y", &timeinfo);

        lcd.clear();
        lcd.showMessage(String(dateStr), 0, 0);
        lcd.showMessage(String(timeStr), 0, 1);
      } else {
        lcd.clear();
        lcd.showMessage("Gagal ambil waktu", 0, 0);
      }
    }
};

class AccessManager {
  private:
    enum Mode { STANDBY, ADMIN } currentMode = STANDBY;

    Display &lcd;
    FingerprintSensor &fp;
    RFIDHandler &rfid;
    KeypadHandler &keypad;
    RelayHandler &rel;
    WiFiHandler &wifi;
    Preferences prefs;
    
    bool adminRegistered = false;
    unsigned long lastAdminInputTime = 0;
    const unsigned long adminTimeout = 10000; // 10 detik

  public:
    // Constructor
    AccessManager(Display &d, FingerprintSensor &f, RFIDHandler &r, KeypadHandler &k, RelayHandler &rel, WiFiHandler &w)
      : lcd(d), fp(f), rfid(r), keypad(k), rel(rel), wifi(w) {}

    void begin() {
      prefs.begin("access", false);
      adminRegistered = prefs.getBool("admin_ok", false);
      prefs.end();
    }

    void loop() {
      if (currentMode == ADMIN) {
        adminModeLoop();  // Mode admin aktif

        // Timeout 10 detik jika tidak ada input
        if (millis() - lastAdminInputTime > adminTimeout) {
          currentMode = STANDBY;
          lcd.clear();
          lcd.showMessage("Kembali ke", 0, 0);
          lcd.showMessage("Mode Standby", 0, 1);
          delay(1500);
          lcd.clear();
        }
      } else {
        char key = keypad.read();
        if (key == '0') {
          handleZeroKey();  // Masuk admin
        } else {
          // Mode user biasa
          fingerSecurity();
          rfidSecurity();
        }
      }
    }

    void handleZeroKey() {
      String pin = "";
      while (pin.length() < 4) {
        char k = keypad.read();
        if (k >= '0' && k <= '9') {
          pin += k;
        }
      }

      if (pin == "0000") {
        lcd.clear();
        lcd.showMessage("Tempelkan jari", 0, 0);
        lcd.showMessage("admin...", 0, 1);
        delay(1000);

        prefs.begin("access", true);
        bool isAdminRegistered = prefs.getBool("admin_ok", false);
        prefs.end();

        if (!isAdminRegistered) {
          lcd.clear();
          lcd.showMessage("Daftar Jari Admin", 0, 0);
          delay(1000);
          fp.enrollFingerSecure(0);

          prefs.begin("access", false);
          prefs.putBool("admin_ok", true);
          prefs.end();

          lcd.clear();
          lcd.showMessage("Admin Disimpan", 0, 0);
          delay(1500);
        } else {
          lcd.clear();
          lcd.showMessage("Verifikasi Jari...", 0, 0);
          delay(1000);

          int id = fp.readIDStatus();
          if (id == 0) {
            currentMode = ADMIN;
            lastAdminInputTime = millis();
            lcd.clear();
            lcd.showMessage("Mode Admin Aktif", 0, 0);
            delay(2000);
          } else {
            lcd.clear();
            lcd.showMessage("Jari Salah", 0, 0);
            delay(1500);
          }
        }
      } else {
        lcd.clear();
        lcd.showMessage("PIN Salah!", 0, 0);
        delay(1500);
      }

      lcd.clear();
    }
 
    void fingerSecurity() {
      int id = fp.readIDStatus();

      if (id >= 0) {
        lcd.showMessage("Terdeteksi ID: " + String(id), 0, 0);
        rel.activateMain();
        delay(2000);
        rel.deactivateMain();
        lcd.clear();
      }
      else if (id == -1) {
        lcd.showMessage("Sidik Jari tidak", 0, 0);
        lcd.showMessage("terdaftar", 0, 1);
        delay(1500);
        lcd.clear();
      }
    }

    void rfidSecurity() {
      String uid = "";
      int id = rfid.checkUIDAndGetID(uid);

      // Tidak ada kartu → tidak lakukan apa pun
      if (id == -1) return;

      // Kartu tidak terdaftar
      if (id == 0) {
        lcd.clear();
        lcd.showMessage("Kartu tidak", 0, 0);
        lcd.showMessage("terdaftar!", 0, 1);
        delay(1500);
        lcd.clear();
        return;
      }

      // Kartu terdaftar → minta PIN
      lcd.clear();
      lcd.showMessage("Masukkan PIN:", 0, 0);
      String inputPin = "";

      unsigned long startTime = millis();
      const unsigned long timeout = 5000; // 5 detik

      while (inputPin.length() < 4) {
        char key = keypad.read();
        if (key >= '0' && key <= '9') {
          inputPin += key;
          String mask = "";
          for (int i = 0; i < inputPin.length(); i++) mask += "*";
          lcd.showMessage(mask, 0, 1);
        }

        // Cek timeout
        if (millis() - startTime > timeout) {
          lcd.clear();
          lcd.showMessage("Timeout PIN", 0, 0);
          delay(1500);
          lcd.clear();
          return;
        }
      }

      // Ambil dari prefs dalam class
      String storedPin = rfid.getStoredPIN(id);
      inputPin.trim();
      storedPin.trim();

      if (inputPin == storedPin) {
        // Akses diberikan
        lcd.clear();
        lcd.showMessage("Akses Diberikan", 0, 0);
        rel.activateAll(); // RelayHandler aktif
        delay(2000);
        rel.deactivateAll();
        lcd.clear();
      } else {
        // PIN salah
        lcd.clear();
        lcd.showMessage("PIN Salah!", 0, 0);
        delay(1500);
        lcd.clear();
      }
    }

    void adminModeLoop() {
      char key = keypad.read();
      if (key >= '1' && key <= '6') {
        lastAdminInputTime = millis();
        int index = String(key).toInt();
        manageUser(index);
      } else if (key == '*') {
        lastAdminInputTime = millis();
        int id = fp.readIDStatus();
        if (id == 0) {
          lcd.showMessage("Reset Sistem...", 0, 0);
          fp.deleteAll();
          rfid.deleteAll();
          wifi.resetSettings();
          prefs.begin("access", false);
          prefs.putBool("admin_ok", false);
          prefs.end();
          delay(2000);
          ESP.restart();
        } else {
          lcd.showMessage("Admin Gagal", 0, 1);
          delay(1500);
        }
      } else if (key == '#') {
        currentMode = STANDBY;
        lcd.clear();
        lcd.showMessage("Keluar Mode", 0, 0);
        lcd.showMessage("Admin", 0, 1);
        delay(1500);
        lcd.clear();
      }
    }

    void manageUser(int index) {
      lcd.clear();
      String method = rfid.getMethod(index);
      if (method != "none") {
        lcd.showMessage("ID " + String(index) + ": " + method, 0, 0);
        delay(1000);

        lcd.clear();
        lcd.showMessage("1:Ganti 2:Batalkan", 0, 0);
        char decision = 0;
        while (decision != '1' && decision != '2') {
          decision = keypad.read();
        }
        if (decision == '2') {
          lcd.clear();
          lcd.showMessage("Pengaturan ", 0, 0);
          lcd.showMessage("dibatalkan", 0, 1);
          delay(1500);
          lcd.clear();
          return;
        }
        lcd.clear();
        lcd.showMessage("1: Finger", 0, 0);
        lcd.showMessage("2: RFID+PIN", 0, 1);
        char methodKey = 0;
        while (methodKey != '1' && methodKey != '2') {
          methodKey = keypad.read();
        }

        rfid.clearIndex(index);
        fp.deleteFingerprint(index);

        if (methodKey == '1') {
          fp.enrollFingerSecure(index);
          rfid.setMethod(index, "finger");
        } else if (methodKey == '2') {
          rfid.enrollWithPIN(index);
          rfid.setMethod(index, "rfid");
        }
      } else {
        lcd.clear();
        lcd.showMessage("1: Finger", 0, 0);
        lcd.showMessage("2: RFID+PIN", 0, 1);
        char methodKey = 0;
        while (methodKey != '1' && methodKey != '2') {
          methodKey = keypad.read();
        }

        if (methodKey == '1') {
          fp.enrollFingerSecure(index);
          rfid.setMethod(index, "finger");
        } else if (methodKey == '2') {
          rfid.enrollWithPIN(index);
          rfid.setMethod(index, "rfid");
        }
      }

      lcd.clear();
      lcd.showMessage("Data Tersimpan", 0, 0);
      delay(1500);
      lcd.clear();
    }

    String waitForPIN() {
      String pin = "";
      while (pin.length() < 4) {
        char k = keypad.read();
        if (k >= '0' && k <= '9') {
          pin += k;
          String mask = "";
          for (int i = 0; i < pin.length(); i++) {
            mask += "*";
          }
          lcd.showMessage(mask, 0, 1);
        }
      }
      return pin;
    }
};


// variabel global untuk RelayHandler
bool isRelayHandlerActive = false;
unsigned long RelayHandlerStartTime = 0;
const unsigned long RelayHandlerDuration = 5000; // 5 detik

// --- Deklarasi objek secara global ---
Display lcd;
RelayHandler doorRelayHandler;
KeypadHandler keypad(lcd);
RFIDHandler myRfid(lcd, keypad);
FingerprintSensor fp(lcd);
WiFiHandler wifi(lcd);
TimeHandler timeHandler(lcd);
AccessManager accessManager(lcd, fp, myRfid, keypad, doorRelayHandler, wifi);


BLYNK_WRITE(V0) {
  if (param.asInt() == 1 && !isRelayHandlerActive) {
    lcd.clear();
    lcd.showMessage("Akses via Blynk", 0, 0);

    doorRelayHandler.activateAll();
    RelayHandlerStartTime = millis();   // Catat waktu mulai
    isRelayHandlerActive = true;        // Aktifkan flag
  }
}

void monitorRelayHandler() {
  if (isRelayHandlerActive && millis() - RelayHandlerStartTime >= RelayHandlerDuration) {
    doorRelayHandler.deactivateAll();
    lcd.clear();
    lcd.showMessage("Relay Off", 0, 0);
    delay(1500);
    lcd.clear();

    isRelayHandlerActive = false; // Reset flag
  }
}

void setup() {
  // Inisialisasi semua komponen
  Serial.begin(115200);
  lcd.begin();
  Wire.begin();
  keypad.begin();
  wifi.begin();
  wifi.beginBlynk();
  myRfid.begin();
  fp.begin();
  // timeHandler.begin();

  accessManager.begin();
}

void loop() 
  Blynk.run();
  accessManager.loop();
  monitorRelayHandler();
}
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

// variabel global
PCF8575 pcf(0x20); 

class Display {
  private:
    LiquidCrystal_I2C lcd;  // objek LCD sebagai anggota class

    bool busy = false;
    unsigned long lastWriteTime = 0;
    const unsigned long busyDuration = 3000;

    byte lockChar[8] = {
      B01110,
      B10001,
      B10001,
      B11111,
      B11011,
      B11011,
      B11111,
      B00000
    };

    byte unlockChar[8] = {
      B01110,
      B10001,
      B00001,
      B01111,
      B11011,
      B11011,
      B11111,
      B00000
    };

    byte battery25[8] = {
      B01110,
      B10001,
      B10001,
      B10001,
      B10001,
      B10001,
      B10001,
      B11111
    };

    byte battery50[8] = {
      B01110,
      B10001,
      B10001,
      B11111,
      B11111,
      B10001,
      B10001,
      B11111
    };

    byte battery75[8] = {
      B01110,
      B11111,
      B11111,
      B11111,
      B11111,
      B10001,
      B10001,
      B11111
    };

    byte battery100[8] = {
      B01110,
      B11111,
      B11111,
      B11111,
      B11111,
      B11111,
      B11111,
      B11111
    };

  public:
    // Constructor: inisialisasi alamat I2C dan ukuran LCD
    Display() : lcd(0x27, 20, 4) {}

    // Method untuk memulai LCD
    void begin() {
      lcd.init();
      lcd.backlight();
      lcd.createChar(0, lockChar);       // 0
      lcd.createChar(1, unlockChar);     // 1
      lcd.createChar(2, battery25);      // 2
      lcd.createChar(3, battery50);      // 3
      lcd.createChar(4, battery75);      // 4
      lcd.createChar(5, battery100);     // 5
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

    void showLockIcon(uint8_t col, uint8_t row) {
      lcd.setCursor(col, row);
      lcd.write(byte(0));
    }

    void showUnlockIcon(uint8_t col, uint8_t row) {
      lcd.setCursor(col, row);
      lcd.write(byte(1));
    }

    // method tampilkan baterai.
    void showBatteryIcon(int percent, int col, int row) {
      uint8_t iconChar = 2; // default 25%

      if (percent >= 75) iconChar = 5;
      else if (percent >= 50) iconChar = 4;
      else if (percent >= 25) iconChar = 3;
      else iconChar = 2;

      lcd.setCursor(col, row);
      lcd.write(iconChar);
    }

    // Method untuk membersihkan layar
    void clear() {
      lcd.clear();
    }
    
    bool isBusy() {
      return busy && (millis() - lastWriteTime < busyDuration);
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
      digitalWrite(RelayHandlerMain, HIGH);
      digitalWrite(RelayHandlerSecond, HIGH);
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
    PCF8575 &pcf;
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
    KeypadHandler(Display &displayRef, PCF8575 &pcf) : pcf(pcf), lcd(displayRef) {}

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

     // Fungsi untuk membaca integer (0–99) dari keypad
    int readIntFromKeypad(bool display, int col, int row) {
      String input = "";
        unsigned long startTime = millis();
        const unsigned long timeout = 10000; // timeout 10 detik

        while (millis() - startTime < timeout) {
          char key = read(); // ini harus membaca tombol '0' hingga '9'
          if (key >= '0' && key <= '9') {
            input += key;

            // tampilkan di layar saat user input
            if (display) {
              lcd.showMessage(input, col, row);
            }

            // jika sudah dua digit, hentikan
            if (input.length() >= 2) break;
          }
        }

        if (input.length() == 0) return -1;
        return input.toInt();
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

      // Cek koneksi RFID saat inisialisasi
      byte version = rfid.PCD_ReadRegister(rfid.VersionReg);

      lcd.clear();
      lcd.showMessage("RFID", 0, 0);
      if (version == 0x00 || version == 0xFF) {
        lcd.showMessage("Tidak Terkoneksi!", 0, 1);
      } else {
        lcd.showMessage("Terkoneksi", 0, 1);
      }
      delay(2000);
      lcd.clear();
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
    bool enrollWithPIN(int id) {
      String keyUID = "uid" + String(id);
      String keyPIN = "pin" + String(id);
      String keyID  = "id"  + String(id);

      if (prefs.isKey(keyUID.c_str()) || prefs.isKey(keyPIN.c_str()) || prefs.isKey(keyID.c_str())) {
        lcd.clear();
        lcd.showMessage("ID sudah dipakai!", 0, 0);
        delay(1500);
        lcd.clear();
        return false;
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
        return false;
      }

      if (getRegisteredIDFromUID(uid) > 0) {
        lcd.clear();
        lcd.showMessage("Kartu sudah", 0, 0);
        lcd.showMessage("terdaftar", 0, 1);
        delay(1500);
        lcd.clear();
        return false;
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
          return false;
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
      return true;
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

    void testRead() {
      lcd.clear();
      lcd.showMessage("Tempelkan kartu", 0, 0);
      
      unsigned long startTime = millis();
      while (millis() - startTime < 5000) { // timeout 5 detik
        String uid = readUID();
        if (uid != "") {
          lcd.clear();
          lcd.showMessage("UID Terdeteksi:", 0, 0);
          lcd.showMessage(uid, 0, 1);
          delay(3000);
          lcd.clear();
          return;
        }
      }

      lcd.clear();
      lcd.showMessage("Tidak ada kartu", 0, 0);
      lcd.showMessage("terdeteksi", 0, 1);
      delay(2000);
      lcd.clear();
    }

    // Reset total semua UID & PIN
    void deleteAll() {
      for (int i = 1; i <= 50; i++) clearIndex(i);
    }

    void checkConnectionStatus() {
      byte version = rfid.PCD_ReadRegister(rfid.VersionReg);

      if (version == 0x00 || version == 0xFF) {
        lcd.clear();
        lcd.showMessage("RFID", 0, 0);
        lcd.showMessage("Tidak Terkoneksi!", 0, 1);
      } else {
        lcd.clear();
        lcd.showMessage("RFID", 0, 0);
        lcd.showMessage("Terkoneksi", 0, 1);
      }

      delay(2000);  // Tampilkan selama 2 detik
      lcd.clear();
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
    bool enrollFingerSecure(uint8_t id) {
      lcd.clear();
      lcd.showMessage("Daftar ID: " + String(id), 0, 0);
      delay(1000);

      // Cek apakah ID sudah dipakai
      if (finger.loadModel(id) == FINGERPRINT_OK) {
        lcd.showMessage("ID sudah dipakai", 0, 1);
        delay(2000);
        lcd.clear();
        return false;
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
        return false;
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
        return false;
      }

      // Cocokkan template
      if (finger.createModel() != FINGERPRINT_OK) {
        lcd.showMessage("Tidak cocok, ulangi", 0, 1);
        delay(2000);
        lcd.clear();
        return false;
      }

      // Simpan model ke ID
      if (finger.storeModel(id) != FINGERPRINT_OK) {
        lcd.showMessage("Gagal simpan data", 0, 1);
        delay(2000);
        lcd.clear();
        return false;
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
        return false;
      }
      if (finger.fingerSearch() != FINGERPRINT_OK || finger.fingerID != id) {
        lcd.showMessage("Gagal cocok ulang", 0, 1);
        // Hapus data karena hasil verifikasi gagal
        finger.deleteModel(id);
        delay(2000);
        lcd.clear();
        return false;
      }

      // Jika berhasil
      lcd.showMessage("Daftar Berhasil!", 0, 1);
      delay(2000);
      lcd.clear();
      return true;
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

    // Method untuk mengecek apakah sidik jari sudah terdaftar
    bool isFingerprintRegistered(uint8_t id) {
      return finger.loadModel(id) == FINGERPRINT_OK;
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

class IOHandler {
  private:
    PCF8575 &pcf;
    KeypadHandler &keypad;
    Display &lcd;
    const uint8_t BUZZER_PIN = 7;     // PCF8575 pin
    const uint8_t BUTTON_PIN = 25;    // ESP32 pin

    unsigned long lastDebounceTime = 0;
    const unsigned long debounceDelay = 50;
    bool lastButtonState = HIGH;
    bool buttonState = HIGH;
    bool buttonHandled = false;

  public:
    IOHandler(PCF8575 &pcf, KeypadHandler &keypad, Display &lcd) : pcf(pcf), keypad(keypad), lcd(lcd) {}

    void begin() {
      pcf.write(BUZZER_PIN, HIGH); // Default mati
      pinMode(BUTTON_PIN, INPUT_PULLUP);
    }

    void buzzOn() {
      pcf.write(BUZZER_PIN, LOW);
    }

    void buzzOff() {
      pcf.write(BUZZER_PIN, HIGH);
    }

    void handleButton(std::function<void()> onClick) {
      int reading = digitalRead(BUTTON_PIN);

      if (reading != lastButtonState) {
        lastDebounceTime = millis();
        buttonHandled = false;
      }

      if ((millis() - lastDebounceTime) > debounceDelay) {
        if (reading == LOW && !buttonHandled) {
          onClick();            // Trigger fungsi callback
          buttonHandled = true;
        }
      }

      lastButtonState = reading;
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
    void begin(const char* apName = "SmartDoorLock", const char* apPassword = "12345678") {
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

class BatteryMonitor {
  private:
    int adcPin;
    Display &lcd;

    // Kalibrasi: karena pembagi tegangan 1:4
    float calibrationFactor = 4.0;   // untuk scaling ADC menjadi tegangan baterai
    float minVoltage = 9.0;          // Baterai 3S minimal
    float maxVoltage = 12.6;         // Baterai 3S penuh

    unsigned long lastDisplayUpdate = 0;
    unsigned long lastBlynkSend = 0;

    const unsigned long displayInterval = 60000;  // 60 detik
    const unsigned long blynkInterval = 30000;    // 30 detik

    int lastDisplayed = -1;

  public:
    BatteryMonitor(int pin, Display &display) : adcPin(pin), lcd(display) {}

    void begin() {
      pinMode(adcPin, INPUT);
    }

    float readVoltage() {
      const int NUM_SAMPLES = 30;
      long total = 0;
      for (int i = 0; i < NUM_SAMPLES; i++) {
        total += analogRead(adcPin);
        delay(2);
      }
      float avgReading = total / float(NUM_SAMPLES);
      float v_adc = (avgReading / 4095.0) * 3.15;       // Tegangan ADC
      float v_batt = v_adc * calibrationFactor;        // Tegangan baterai hasil scaling
      return v_batt;
    }

    int getPercentage() {
      float voltage = readVoltage();
      float percent = ((voltage - minVoltage) / (maxVoltage - minVoltage)) * 100.0;
      percent = constrain(percent, 0, 100);
      return round(percent);
    }

    void updateDisplayIfNeeded() {
        unsigned long now = millis();
        if (now - lastDisplayUpdate >= displayInterval) {
          int percent = getPercentage();
          if (percent != lastDisplayed) {
            lcd.showMessage(String(percent) + "%", 12, 0); // posisi di kanan atas
            lastDisplayed = percent;
          }
          lastDisplayUpdate = now;
        }
      }

    void sendToBlynkIfNeeded() {
      unsigned long now = millis();
      if (now - lastBlynkSend >= blynkInterval) {
        int percent = getPercentage();
        Blynk.virtualWrite(V2, percent); // Ganti V2 jika perlu
        lastBlynkSend = now;
      }
    }
};

class TimeHandler {
  private:
    const char* ntpServer = "pool.ntp.org";
    const long gmtOffset_sec = 7 * 3600;  // GMT+7 WIB
    const int daylightOffset_sec = 0;
    Display &lcd;
    BatteryMonitor &battery;
    Preferences prefs;

    unsigned long lastUpdate = 0;
    int lastDisplayedMinute = -1;


  public:
    // Constructor dengan referensi ke objek Display
    TimeHandler(Display &displayRef,  BatteryMonitor &battery) :
      lcd(displayRef), battery(battery) {}

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

    void updateClock(bool lcdAvailable = true) {
      if (!lcdAvailable) return;

      unsigned long currentMillis = millis();
      if (currentMillis - lastUpdate < 1000) return;  // Cek setiap 1 detik
      lastUpdate = currentMillis;

      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        if (timeinfo.tm_min != lastDisplayedMinute) {
          lastDisplayedMinute = timeinfo.tm_min;

          // Format waktu
          char timeStr[6];
          sprintf(timeStr, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
          lcd.showMessage(String(timeStr), 6, 0);  // Menampilkan waktu

          // Hapus jejak persen jika ada
          lcd.showMessage("   ", 12, 0);  // overwrite area ikon baterai

          // Tampilkan ikon baterai
          int batteryPercent = battery.getPercentage();
          lcd.showBatteryIcon(batteryPercent, 12, 0);

          // Tampilkan ikon kunci tetap
          lcd.showLockIcon(8, 1);
        }
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
    TimeHandler &time;
    IOHandler io;
    BatteryMonitor &battery;
    Preferences prefs;
    
    // Admin mode logic
    bool adminRegistered = false;
    unsigned long lastAdminInputTime = 0;
    const unsigned long adminTimeout = 20000; // 10 detik

    // Relay handler logic
    bool isRelayHandlerActive = false;
    unsigned long relayStartTime = 0;
    unsigned long relayDuration = 3000;  // default 3 detik
    
    // Metode akses
    bool isBusy = false;

    // variabel night mode
    int jamMalamMulai = 22; // default 16:00
    int jamMalamSelesai = 5; // default 05:00

    // variabel rfid
    String lastUID = "";
    int pinFailCount = 0;
  public:
    // Constructor
    AccessManager(Display &d, FingerprintSensor &f, RFIDHandler &r, KeypadHandler &k, RelayHandler &rel, WiFiHandler &w, TimeHandler &time, IOHandler &io, BatteryMonitor &battery)
      : lcd(d), fp(f), rfid(r), keypad(k), rel(rel), wifi(w), time(time), io(io), battery(battery) {}

    // method untuk memulai AccessManager
    void begin() {
      prefs.begin("access", false);
      adminRegistered = prefs.getBool("admin_ok", false) && fp.isFingerprintRegistered(0);
      prefs.end();
      loadJamMalamPrefs();
    }

    // method loop
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
          isBusy = false;
          
          // Cek input dari metode akses
          if (fingerSecurity()) isBusy = true;
          if (rfidSecurity()) isBusy = true;
          if (handlePhysicalButton()) isBusy = true;
          // battery.sendToBlynkIfNeeded(V5);
          battery.updateDisplayIfNeeded();  // Saat standby
          battery.sendToBlynkIfNeeded();           // Kirim tiap 30 detik

          // Tampilkan jam hanya jika LCD tidak sedang dipakai
          time.updateClock(!isBusy);
        }
      }
      monitorRelayTimeout();
    }

//---------Method Admin Operations----------------
    // method untuk mengelola masuk admin mode
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
        lcd.showMessage("Memeriksa admin...", 0, 0);
        delay(3000);

        prefs.begin("access", true);
        bool isAdminRegistered = prefs.getBool("admin_ok", false);
        prefs.end();

        bool fingerExist = fp.isFingerprintRegistered(0); // CEK REAL DATA

        if (!isAdminRegistered || !fingerExist) {
          // Reset flag jika ada inkonsistensi
          prefs.begin("access", false);
          prefs.putBool("admin_ok", false);
          prefs.end();

          // Daftarkan admin
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
          lcd.clear();
          currentMode = STANDBY;  // Kembali ke mode standby
        }

        // Jika admin valid → verifikasi
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
      } else {
        lcd.clear();
        lcd.showMessage("PIN Salah!", 0, 0);
        delay(1500);
      }

      lcd.clear();
    }

    // Method untuk mengelola mode admin
    void adminModeLoop() {
        static bool firstEntry = true;
        if (firstEntry) {
          lcd.clear();
          lcd.showMessage("1-6: User", 0, 0);
          lcd.showMessage("7: Jam Malam", 0, 1);
          firstEntry = false;
        }
      char key = keypad.read();
      if (key >= '1' && key <= '6') {
        lastAdminInputTime = millis();
        int index = String(key).toInt();
        manageUser(index);
      } if (key == '7') {
          firstEntry = true;
          configureJamMalam();
        } else if (key == '9') {
            lastAdminInputTime = millis();  // Reset timeout
            tampilkanIDTerdaftar();
          } 
      else if (key == '*') {
          lastAdminInputTime = millis();

          lcd.clear();
          lcd.showMessage("Tempelkan jari", 0, 0);
          lcd.showMessage("admin untuk reset", 0, 1);

          unsigned long start = millis();
          int id = -1;
          bool success = false;

          // Tunggu hingga 2 detik untuk membaca sidik jari
          while (millis() - start < 2000) {
            id = fp.readIDStatus();
            if (id == 0) {
              success = true;
              break;
            }
          }

          if (success) {
            lcd.clear();
            lcd.showMessage("Reset Sistem...", 0, 0);

            fp.deleteAll();
            rfid.deleteAll();
            wifi.resetSettings();

            // Reset flag admin di Preferences
            prefs.begin("access", false);
            prefs.clear();
            prefs.end();

            lcd.showMessage("Sistem direset", 0, 1);
            delay(2000);
            lcd.clear();
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

    // Method Admin untuk mengelola pengguna
    void manageUser(int index) {
      lcd.clear();
      String method = rfid.getMethod(index);

      if (method == "finger" || method == "rfid") {
        lcd.showMessage("ID " + String(index) + ": " + method, 0, 0);
        delay(1000);

        lcd.clear();
        lcd.showMessage("1:Ganti 2:Batalkan", 0, 0);
        lcd.showMessage("3:Hapus", 0, 1);

        char decision = 0;
        while (decision != '1' && decision != '2' && decision != '3') {
          decision = keypad.read();
        }

        if (decision == '2') {
          lcd.clear();
          lcd.showMessage("Pengaturan", 0, 0);
          lcd.showMessage("Dibatalkan", 0, 1);
          delay(1500);
          lcd.clear();
          return;
        }

        if (decision == '3') {
          lcd.clear();
          lcd.showMessage("1:Hapus 2:Batalkan", 0, 0);
          char confirm = 0;
          while (confirm != '1' && confirm != '2') {
            confirm = keypad.read();
          }

          if (confirm == '1') {
            rfid.clearIndex(index);
            fp.deleteFingerprint(index);
            rfid.setMethod(index, "none");

            lcd.clear();
            lcd.showMessage("Data Dihapus", 0, 0);
            delay(1500);
            lcd.clear();
            return;
          } else {
            lcd.clear();
            lcd.showMessage("Penghapusan", 0, 0);
            lcd.showMessage("Dibatalkan", 0, 1);
            delay(1500);
            lcd.clear();
            return;
          }
        }

        // Jika memilih GANTI
        lcd.clear();
        lcd.showMessage("1:Finger 3:Batalkan", 0, 0);
        lcd.showMessage("2:RFID+PIN", 0, 1);

        char methodKey = 0;
        while (methodKey != '1' && methodKey != '2' && methodKey != '3') {
          methodKey = keypad.read();
        }

        if (methodKey == '3') {
          lcd.clear();
          lcd.showMessage("Pengaturan", 0, 0);
          lcd.showMessage("Dibatalkan", 0, 1);
          delay(1500);
          lcd.clear();
          return;
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
        // Saat belum terdaftar
        lcd.clear();
        lcd.showMessage("1:Finger 3:Batalkan", 0, 0);
        lcd.showMessage("2:RFID+PIN", 0, 1);

        char methodKey = 0;
        while (methodKey != '1' && methodKey != '2' && methodKey != '3') {
          methodKey = keypad.read();
        }

        if (methodKey == '3') {
          lcd.clear();
          lcd.showMessage("Pendaftaran", 0, 0);
          lcd.showMessage("Dibatalkan", 0, 1);
          delay(1500);
          lcd.clear();
          return;
        }

        if (methodKey == '1') {
          if (fp.enrollFingerSecure(index)) {
            rfid.setMethod(index, "finger");
          } else {
            lcd.clear();
            lcd.showMessage("Pendaftaran gagal", 0, 0);
            delay(1500);
            lcd.clear();
            return;
          }
        } else if (methodKey == '2') {
          if (rfid.enrollWithPIN(index)) {
            rfid.setMethod(index, "rfid");
          } else {
            lcd.clear();
            lcd.showMessage("Pendaftaran gagal", 0, 0);
            delay(1500);
            lcd.clear();
            return;
          }
        }
      }

      lcd.clear();
      lcd.showMessage("Data Tersimpan", 0, 0);
      delay(1500);
      lcd.clear();
    }

//---------method jam malam-------------------
    int getCurrentHour() {
      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        return timeinfo.tm_hour;
      }
      return -1; // gagal
    }

    bool isNightTime() {
      int now = getCurrentHour();
      if (now == -1) return false;  // Gagal mendapatkan waktu

      if (jamMalamMulai > jamMalamSelesai) {
        // Contoh: 16 sampai 5 (lewat tengah malam)
        return (now >= jamMalamMulai || now < jamMalamSelesai);
      } else {
        // Contoh: 22 sampai 23
        return (now >= jamMalamMulai && now < jamMalamSelesai);
      }
    }

    void saveJamMalamPrefs() {
      prefs.begin("access", false);
      prefs.putInt("jam_mulai", jamMalamMulai);
      prefs.putInt("jam_selesai", jamMalamSelesai);
      prefs.end();
    }

    // method load jam malam
    void loadJamMalamPrefs() {
      prefs.begin("access", true);
      jamMalamMulai = prefs.getInt("jam_mulai", 16); // default 16
      jamMalamSelesai = prefs.getInt("jam_selesai", 5); // default 5
      prefs.end();
    }
    
    // method mengatur jam malam
    void configureJamMalam() {
      lcd.clear();
      lcd.showMessage("Atur Jam Malam", 0, 0);
      delay(1000);
      lcd.clear();

      lcd.showMessage("Mulai (0-23):", 0, 0);
      int mulai = keypad.readIntFromKeypad(true, 0, 1); 
      delay(1000);
      if (mulai == -1) return;
      lcd.clear();

      lcd.clear();
      lcd.showMessage("Selesai (0-23):", 0, 0);
      int selesai = keypad.readIntFromKeypad(true, 0, 1);
      delay(1000);
      if (selesai == -1) return;
      lcd.clear();

      if (mulai >= 0 && mulai <= 23 && selesai >= 0 && selesai <= 23) {
        jamMalamMulai = mulai;
        jamMalamSelesai = selesai;

        lcd.clear();
        lcd.showMessage("Menyimpan...", 0, 0);
        delay(500);  // jeda tampilan sebelum simpan
        saveJamMalamPrefs();

        lcd.clear();
        lcd.showMessage("Jam malam disimpan", 0, 0);
      } else {
        lcd.clear();
        lcd.showMessage("Input tidak valid", 0, 0);
      }

      delay(2000);
      lcd.clear();
    }

//---------Method untuk mengelola akses relay----------------
    // Method untuk mengaktifkan relay dengan timeout
    void activateRelayWithTimeout(unsigned long duration = 3000) {
      rel.activateAll();
      relayStartTime = millis();
      lcd.clear();
      lcd.showMessage("Unlocked", 5, 0);
      lcd.showUnlockIcon(8, 1);
      delay(3500);
      relayDuration = duration;
      isRelayHandlerActive = true;
      Blynk.virtualWrite(V3, 0); // tetap nyala
      Blynk.setProperty(V3, "color", "#FF0000"); 
      Blynk.virtualWrite(V4, 255); // mati
      Blynk.setProperty(V4, "color", "#00FF00"); 
    }
    
    // Method untuk memantau timeout relay kemudian menonaktifkannya
    void monitorRelayTimeout() {
      if (isRelayHandlerActive && millis() - relayStartTime >= relayDuration) {
        rel.deactivateSecond();
        delay(500);  
        rel.deactivateMain();
        lcd.clear();
        lcd.showMessage("Locked", 5, 0);
        lcd.showLockIcon(8, 1);
        delay(3000);
        lcd.clear();
        isRelayHandlerActive = false;
        Blynk.virtualWrite(V4, 0); // tetap nyala
        Blynk.setProperty(V4, "color", "#00FF00"); 
        Blynk.virtualWrite(V3, 255); // mati
        Blynk.setProperty(V3, "color", "#FF0000"); 
      }
    }

//---------Method untuk akses Blynk, sidik jari, RFID, dan tombol fisik----------------
    // Method untuk akses Blynk
    bool blynkAccess() {
      if (!isRelayHandlerActive) {
        lcd.clear();
        lcd.showMessage("Akses Blynk", 0, 0);
        activateRelayWithTimeout(4000);
        return true;
      }
      return false;  // Relay sudah aktif
    }

    // Method untuk menangani keamanan sidik jari
    bool fingerSecurity() {
      int id = fp.readIDStatus();

      if (id >= 0) {
        lcd.showMessage("Terdeteksi ID: " + String(id), 0, 0);
        activateRelayWithTimeout(4000);
        lcd.clear();
        return true;
      }
      else if (id == -1) {
        lcd.showMessage("Sidik Jari tidak", 0, 0);
        lcd.showMessage("terdaftar", 0, 1);
        delay(1500);
        lcd.clear();
        return true;
      }
      return false;
    }
    
    // Method untuk menangani keamanan RFID
    bool rfidSecurity() {
      String uid = "";
      if (uid == lastUID) {
        // UID yang sama, teruskan counter
      } else {
        lastUID = uid;
        pinFailCount = 0;  // Reset jika UID berubah
      }
      int id = rfid.checkUIDAndGetID(uid);

      // Tidak ada kartu → tidak lakukan apa pun
      if (id == -1) return false;

      // cek jam malam
      if (isNightTime()) {
        lcd.clear();
        lcd.showMessage("Jam malam aktif", 0, 0);
        lcd.showMessage("Akses RFID ditolak", 0, 1);
        delay(2000);
        lcd.clear();
        return false;
      }

      // Kartu tidak terdaftar
      if (id == 0) {
        lcd.clear();
        lcd.showMessage("Kartu tidak", 0, 0);
        lcd.showMessage("terdaftar!", 0, 1);
        delay(1500);
        lcd.clear();
        return true;
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
          return true;
        }
      }

      // Ambil dari prefs dalam class
      String storedPin = rfid.getStoredPIN(id);
      inputPin.trim();
      storedPin.trim();

      if (inputPin == storedPin) {
        // Akses diberikan
        pinFailCount = 0;
        lcd.clear();
        lcd.showMessage("Akses Diberikan", 0, 0);
        activateRelayWithTimeout(4000);
        lcd.clear();
      } else {
        // PIN salah
        pinFailCount++;
        lcd.clear();
        lcd.showMessage("PIN Salah!", 0, 0);
        delay(1500);
        lcd.clear();
        if (pinFailCount >= 3) {
          io.buzzOn();
          delay(3000);
          io.buzzOff();
          Blynk.logEvent("pin_gagal", "Percobaan PIN 3x gagal - UID: ");
          pinFailCount = 0;  // Reset setelah trigger buzzer
        }
        return true;  // PIN salah, akses ditolak
      }
      return false;  // Akses berhasil
    }

    // Method untuk menangani tombol fisik
    bool handlePhysicalButton() {
      bool wasActivated = false;
      io.handleButton([&]() {
        activateRelayWithTimeout(5000);
        wasActivated = true;
      });
      return wasActivated;
    }

    // method tampilkan ID terdaftar
    void tampilkanIDTerdaftar() {
      lcd.clear();
      lcd.showMessage("Terdaftar ID:", 0, 0);

      String daftar = "";
      for (int i = 1; i <= 6; i++) {
        if (rfid.getMethod(i) != "none") {
          daftar += String(i) + " ";
        }
      }

      if (daftar.length() == 0) {
        lcd.showMessage("Tidak ada", 0, 1);
      } else {
        lcd.showMessage(daftar, 0, 1);
      }

      delay(3000);
      lcd.clear();
    }

    //merubah PIN yang dimasukkan menjadi ****
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

class EmergencyResetHandler {
  private:
    String buffer = "";
    KeypadHandler &keypad;
    Display &lcd;
    WiFiHandler &wifi;
    FingerprintSensor &fp;
    RFIDHandler &rfid;
    Preferences prefs;

    const String resetAllCode = "11111";
    const String resetFPCode  = "22222";
    const String resetWiFiCode = "33333";

  public:
    EmergencyResetHandler(KeypadHandler &keypadRef, Display &lcdRef, WiFiHandler &wifiRef, FingerprintSensor &fpRef, RFIDHandler &rfidRef)
      : keypad(keypadRef), lcd(lcdRef), wifi(wifiRef), fp(fpRef), rfid(rfidRef) {}

    void checkResetCode() {
      char key = keypad.read();
      if (key >= '0' && key <= '9') {
        buffer += key;
        if (buffer.length() > 5) {
          buffer = buffer.substring(buffer.length() - 5);  // Batasi buffer ke 5 digit terakhir
        }

        if (buffer == resetAllCode) {
          lcd.clear();
          lcd.showMessage("Reset Semua", 0, 0);
          lcd.showMessage("Berlangsung...", 0, 1);
          delay(1500);

          fp.deleteAll();
          rfid.deleteAll();
          wifi.resetSettings();
          prefs.clear();

          lcd.clear();
          lcd.showMessage("Semua Reset!", 0, 0);
          delay(2000);
          ESP.restart();
        }
        else if (buffer == resetFPCode) {
          lcd.clear();
          lcd.showMessage("Reset Fingerprint", 0, 0);
          lcd.showMessage("Berlangsung...", 0, 1);
          delay(1500);

          fp.deleteAll();

          lcd.clear();
          lcd.showMessage("Fingerprint", 0, 0);
          lcd.showMessage("Telah Direset!", 0, 1);
          delay(2000);
          ESP.restart();
        }
        else if (buffer == resetWiFiCode) {
          lcd.clear();
          lcd.showMessage("Reset WiFi", 0, 0);
          lcd.showMessage("Berlangsung...", 0, 1);
          delay(1500);

          wifi.resetSettings();

          lcd.clear();
          lcd.showMessage("WiFi Direset!", 0, 0);
          delay(2000);
          ESP.restart();
        }
      }
    }
};

// --- Deklarasi objek secara global ---
Display lcd;
RelayHandler doorRelayHandler;
KeypadHandler keypad(lcd, pcf);
RFIDHandler myRfid(lcd, keypad);
FingerprintSensor fp(lcd);
IOHandler ioHandler(pcf, keypad, lcd);
WiFiHandler wifi(lcd);
BatteryMonitor battery(34, lcd);
TimeHandler timeHandler(lcd, battery);
AccessManager accessManager(lcd, fp, myRfid, keypad, doorRelayHandler, wifi, timeHandler, ioHandler, battery);
EmergencyResetHandler emergencyReset(keypad, lcd, wifi, fp, myRfid);

// akses via Blynk
BLYNK_WRITE(V0) {
  accessManager.blynkAccess();
}

void setup() {
  Serial.begin(115200);
  lcd.begin();
  Wire.begin();
  keypad.begin();
  wifi.begin();
  wifi.beginBlynk();
  myRfid.begin();
  fp.begin();
  ioHandler.begin(); 
  timeHandler.begin();
  battery.begin();
  accessManager.begin();
}

void loop() {
  emergencyReset.checkResetCode();  
  Blynk.run();
  accessManager.loop();
  // myRfid.testRead();
  // myRfid.checkConnectionStatus();
  // keypad.testKeypad();
  // fp.testCapture();
}
#include <Arduino.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <PCF8575.h>
#include <Adafruit_Fingerprint.h>
#include <WiFi.h>
#include <WiFiManager.h>  // oleh tzapu
#include <Preferences.h>       // untuk penyimpanan token Blynk
#define FINGERPRINT_LEDON           0x01
#define FINGERPRINT_LEDOFF          0x02
#define FINGERPRINT_LEDBREATHING    0x03
#define FINGERPRINT_LEDRED          0x04
#define FINGERPRINT_LEDGREEN        0x05
#define FINGERPRINT_LEDBLUE         0x06
#define FINGERPRINT_LEDYELLOW       0x07
#define FINGERPRINT_LEDCYAN         0x08
#define FINGERPRINT_LEDMAGENTA      0x09
#define FINGERPRINT_LEDWHITE        0x0A

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

class Relay {
  private:
    const int relayMain   = 26;
    const int relaySecond = 27;

  public:
    // Constructor: set mode
    Relay() {
      pinMode(relayMain, OUTPUT);
      pinMode(relaySecond, OUTPUT);
    }

    // Method untuk mengetes kedua relay
    void testRelay() {
      digitalWrite(relayMain, HIGH);
      digitalWrite(relaySecond, HIGH);
      delay(500);
      
      digitalWrite(relayMain, LOW);
      digitalWrite(relaySecond, LOW);
      delay(500);

      digitalWrite(relayMain, HIGH);
      digitalWrite(relaySecond, HIGH);
      delay(500);

      digitalWrite(relayMain, LOW);
      digitalWrite(relaySecond, LOW);
      delay(500);

      digitalWrite(relayMain, HIGH);
      digitalWrite(relaySecond, HIGH);
    }

    // Method untuk mengaktifkan relay
    void activateMain() {
      digitalWrite(relayMain, LOW);  // tergantung tipe relay, aktif bisa LOW
    }

    void activateSecond() {
      digitalWrite(relaySecond, LOW);
    }

    // Method untuk menonaktifkan relay
    void deactivateMain() {
      digitalWrite(relayMain, HIGH);
    }

    void deactivateSecond() {
      digitalWrite(relaySecond, HIGH);
    }

    // Matikan kedua relay
    void deactivateAll() {
      digitalWrite(relayMain, HIGH);
      digitalWrite(relaySecond, HIGH);
    }

    // Aktifkan kedua relay
    void activateAll() {
      digitalWrite(relayMain, LOW);
      digitalWrite(relaySecond, LOW);
    }
};

class RFIDReader {
  private:
    const uint8_t SS_PIN  = 15;
    const uint8_t RST_PIN = 5;
    MFRC522 rfid;

    Display &lcd;

  public:
    // Constructor: inisialisasi objek MFRC522
    RFIDReader(Display &displayRef) : rfid(SS_PIN, RST_PIN), lcd(displayRef) {}

    // Inisialisasi modul RFID
    void begin() {
      SPI.begin(18, 19, 23, SS_PIN); // SCK, MISO, MOSI, SS
      rfid.PCD_Init();
    }

    // Fungsi untuk test dengan menampilkan ke LCD
    void testRfid() {
      lcd.clear();
      lcd.showMessage("Tempelkan Kartu:", 0, 0);
      delay(800);

      // Cek apakah ada kartu baru
      if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

      // Buat string UID
      String uidString = "";
      for (byte i = 0; i < rfid.uid.size; i++) {
        if (rfid.uid.uidByte[i] < 0x10) uidString += "0";
        uidString += String(rfid.uid.uidByte[i], HEX);
        if (i < rfid.uid.size - 1) uidString += " ";
      }

      // Tampilkan UID ke LCD
      lcd.clear();
      lcd.showMessage("Kartu Terdeteksi:", 0, 0);
      lcd.showMessage(uidString, 2, 1);
      delay(2000);
      lcd.clear();

      // Stop komunikasi
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      delay(1500);
    }

    // Ambil UID kartu sebagai String (tanpa LCD)
    String readUID() {
      if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return "";

      String uidString = "";
      for (byte i = 0; i < rfid.uid.size; i++) {
        if (rfid.uid.uidByte[i] < 0x10) uidString += "0";
        uidString += String(rfid.uid.uidByte[i], HEX);
        if (i < rfid.uid.size - 1) uidString += " ";
      }

      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
      return uidString;
    }

    // Cek apakah kartu ada
    bool isCardPresent() {
      return rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial();
    }
};

class KeypadCustom {
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
    KeypadCustom(Display &displayRef) : pcf(0x20), lcd(displayRef) {}

    // Inisialisasi PCF dan I/O
    void begin() {
      while (!pcf.begin()) {
        Serial.println("PCF8575 Error!");
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

    // Method pengetesan sederhana - ambil 1 gambar jari
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

    // Method pembacaan ID jika ada jari
    int readID() {
      if (finger.getImage() != FINGERPRINT_OK) return -1;
      if (finger.image2Tz() != FINGERPRINT_OK) return -1;      
      if (finger.fingerSearch() != FINGERPRINT_OK) return -1;

      return finger.fingerID;
    }

    // Method debugging: tampilkan ID ke display
    void testMatch() {
      lcd.clear();
      lcd.showMessage("Tempelkan jari...", 0, 0);
      delay(500);
      int id = readID();
      if (id >= 0) {
        lcd.showMessage("ID ditemukan: " + String(id), 0, 1);
      } else {
        lcd.showMessage("ID tidak ditemukan", 0, 1);
      }

      delay(2000);
      lcd.clear();
    }

    // Method status jumlah template terdaftar
    void printTemplateCount() {
      finger.getTemplateCount();
      lcd.clear();
      lcd.showMessage("Template: " + String(finger.templateCount), 0, 0);
      delay(2000);
      lcd.clear();
    }

    // Method untuk menghapus seluruh template
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
    WiFiHandler(Display &displayRef) : custom_blynk("blynk", "Blynk Token", "", 33), lcd(displayRef) {}

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
      lcd.showMessage(WiFi.localIP().toString(), 0, 1);
      delay(2000);
      lcd.clear();
      
      //  Simpan token ke preferences
      strncpy(blynkToken, custom_blynk.getValue(), sizeof(blynkToken));
      prefs.begin("blynk", false);  // namespace = blynk
      prefs.putString("token", blynkToken);
      prefs.end();

      Serial.println("Token Blynk disimpan: " + String(blynkToken));
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
      lcd.showMessage("SSID: " + WiFi.SSID(), 0, 0);
      delay(2000);
      lcd.clear();
    }

    // Menampilkan IP saat ini
    String getIP() {
      return WiFi.localIP().toString();
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

// --- Deklarasi objek secara global ---
Display myDisplay;
Relay doorRelay;
RFIDReader myRfid(myDisplay);
KeypadCustom keypad(myDisplay);
FingerprintSensor fp(myDisplay);
WiFiHandler wifi(myDisplay);

void setup() {
  Serial.begin(115200);
  myDisplay.begin();  
  myRfid.begin();
  keypad.begin();
  fp.begin();
  wifi.begin();

}

void loop() {

  char key = keypad.read();
  if (key == '1') {
    // fp.enrollFingerSecure(1); 
    wifi.getSSID(); // tampilkan SSID yang terkoneksi
  } 
  //if (key == '2') {
  //   fp.enrollFingerSecure(2); 
  // } 
  // if (key == '3') {
  //   fp.enrollFingerSecure(3); 
  // } 
  // if (key == '4') {
  //   fp.enrollFingerSecure(4); 
  // } 
  // if (key == '5') {
  //   fp.printTemplateCount(); 
  // } 
  // if (key == '6') {
  //   fp.testMatch();
  // } 
  // if (key == '7') {
  //   fp.readID();
  // } 
  // if (key == '8') {
  //   fp.testCapture();
  // } 
  // if (key == '9') {
  //   fp.testCapture();
  // } 
  if (key == '*') {
    // fp.deleteAll();
    wifi.resetSettings(); // reset WiFi settings
  }
  // int id = fp.readID();
  // if (id >= 1) {
  //   doorRelay.activateAll();
  //   delay(3000);
  // } else {
  //   doorRelay.deactivateAll();
  // }
}


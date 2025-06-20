#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <PCF8575.h>

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
    // Constructor: set mode dan inisialisasi kondisi awal
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

  public:
    // Constructor: inisialisasi objek MFRC522
    RFIDReader() : rfid(SS_PIN, RST_PIN) {}

    // Inisialisasi modul RFID
    void begin() {
      SPI.begin(18, 19, 23, SS_PIN); // SCK, MISO, MOSI, SS
      rfid.PCD_Init();
    }

    // Fungsi untuk test dengan menampilkan ke LCD
    void testRfid(Display &lcd) {
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
      delay(1500);
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

  public:
    // Constructor
    KeypadCustom() : pcf(0x20) {}

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
    void testKeypad(Display &lcd) {
      char key = read();
      if (key != '\0') {
        lcd.clear();
        lcd.showMessage("Tombol ditekan:", 0, 0);
        lcd.showMessage(String(key), 7, 1);
      }
    }
};

// --- Deklarasi objek secara global ---
Display myDisplay;
Relay doorRelay;
RFIDReader myRfid;
KeypadCustom keypad;

void setup() {
  Serial.begin(115200);
  myDisplay.begin();  
  myRfid.begin();
  keypad.begin();

  // myDisplay.testLcd();
  // doorRelay.testRelay();
}

void loop() {
  // myRfid.testRfid(myDisplay);
  keypad.testKeypad(myDisplay);
  delay(100);                  
}


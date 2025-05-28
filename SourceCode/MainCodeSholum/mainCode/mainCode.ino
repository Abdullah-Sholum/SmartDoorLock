// tambahkan library ke code
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <PCF8575.h>
#include <LiquidCrystal_I2C.h>

// ==================================Inisiasi hardware, pinout, etc==================================
// daftarkan alamat lcd beserta ukuran row & kolom
// inisiasi objcet lcd dengan class LiquidCrystal_I2C dengan parameter alamat lcd, jumlah kolom & jumlah row
LiquidCrystal_I2C lcd(0x27,20,4);

// Inisialisasi PCF8575 di alamat 0x20
PCF8575 pcf(0x20);

// inisiasi pin untuk relay 1 & 2
const int relayMain   = 26;
const int relaySecond = 27;

// inisiasi pin untuk RFID | MFRC522
// inisiasi rfid dengan parameter berupa pin rst & ss
const int SS_PIN  = 15;  // SDA
const int RST_PIN = 5;  // RST
MFRC522 rfid(SS_PIN, RST_PIN);

// inisiasi btn, kondisi btn ke kondisi "false", kondisi terakhir btn, waktu terakhir debounce & delay debounce & nilai bool firstRun
const int btn = 25;
int btnState = 1;
int lastBtnState = 1;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50; 
bool firstRun = true;

// inisiasi mySerial dengan hardwareSerial untuk membuat mySerial sebagai UART 2
// inisiasi object finger dengan class Adafruit_Fingerprint dengan parameter berupa semua parameter yang diset ke-mySerial
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// Keypad 4x3
// set Rows & Cols 
// buat array 2 dimensi dengen ukuran Rows x Cols, dengan isi berupa mapping keypad
// mapping pin kedalam array berdasarkan jumlah Rows & Cols
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
// Pin yang terhubung ke PCF8575
byte rowPins[ROWS] = {0, 1, 2, 3}; // P0-P3 sebagai baris (input)
byte colPins[COLS] = {6, 5, 4};    // P4-P6 sebagai kolom (output)
// ==================================end of Inisiasi hardware, pinout, etc==================================


// ==================================Fungsi Fungsionalitas==================================
// Fungsi untuk membaca keypad
// gunakan char karena mengembalikan nilai int
// 
char readKeypad() {
  for (byte c = 0; c < COLS; c++) {
    // Aktifkan kolom saat ini (LOW)
    pcf.write(colPins[c], LOW);

    // Baca semua baris
    for (byte r = 0; r < ROWS; r++) {
      if (pcf.read(rowPins[r]) == LOW) {  // Jika baris LOW (tombol ditekan)
        char key = keys[r][c];
        // Tunggu debounce dan lepas tombol
        delay(50);
        while (pcf.read(rowPins[r]) == LOW);
        
        // Nonaktifkan kolom sebelum return
        pcf.write(colPins[c], HIGH);
        return key;
      }
    }

    // Nonaktifkan kolom (HIGH)
    pcf.write(colPins[c], HIGH);
  }
  
  return '\0'; // Tidak ada tombol ditekan
}
// ==================================end of Fungsi Fungsionalitas==================================

// ==================================fungsi Debug==================================
// fungsi print rfid ke lcd
void cetakRfid() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tempelkan Kartu:");
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

  // Tampilkan di LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Kartu Terdeteksi:");
  lcd.setCursor(2, 1);
  lcd.print(uidString);
  delay(1500);
  lcd.clear();

  // Stop komunikasi dengan kartu
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(1500);  // Tunggu sebentar sebelum membaca lagi
}

// fungsi mengetest btn
void testBtn() {
  int reading = digitalRead(btn);

  // Deteksi perubahan status tombol
  if (reading != lastBtnState) {
    lastDebounceTime = millis();
  }

  // Jika stabil lebih dari debounceDelay
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Jika status tombol berubah
    if (reading != btnState) {
      btnState = reading;

      lcd.clear();
      lcd.setCursor(1, 0);

      if (btnState == LOW) {
        lcd.print("Tombol ditekan");
      } else {
        lcd.print("Tombol dilepas");
      }
    }
  }

  // Saat pertama kali program berjalan
  if (firstRun) {
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("Tekan tombol");
    firstRun = false;
  }

  lastBtnState = reading;
}

// test fingerprint
void testFinger() {
  // set timeout durasi finger jika kondisi tidak ada input ketika pencocokan
  // set startime
  // buat id untuk menyimpan data
  unsigned long timeoutDuration = 3000;  
  unsigned long startTime;
  int getFingerID = 1;           
  // Step 1: Cek koneksi fingerprint dengan melakukan verivikasi password internal modul
  if (finger.verifyPassword()) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fingerprint");
    lcd.setCursor(0, 1);
    lcd.print("Terhubung");
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fingerprint");
    lcd.setCursor(0, 1);
    lcd.print("error!");
    delay(1000);
    return;
  }
  delay(800);

  // Step 2: Minta tempelkan jari
  // ketika kondisi true / finger terhubung maka eksekusi
  while (true) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Tempelkan jari");
    startTime = millis();

    // Tunggu sidik jari dengan timeout
    // cek kondisi jika finger gagal mengambil data, kemudian ulangi step. lanjut step jika finger berhasil menangkap data
    while (finger.getImage() != FINGERPRINT_OK) {
      if (millis() - startTime > timeoutDuration) {
        finger.LEDcontrol(FINGERPRINT_LED_RED, FINGERPRINT_LED_FLASHING, 25, 3);  // LED merah kedip 3x
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Timeout! Ulangi");
        delay(3000);
        return;  // restart dari awal fungsi
      }
    }

    // Step 3: Ambil image dan buat template
    // convert data menjadi template lalu simpan di slot 1 & cek jika tidak berhasil maka ulangi dari awal
    if (finger.image2Tz(1) != FINGERPRINT_OK) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Gagal ubah img");
      delay(1000);
      continue;
    }

    // Minta angkat jari
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Angkat jari...");
    while (finger.getImage() != FINGERPRINT_NOFINGER);

    // Step 4: Scan ulang
    // pada tahap re scan jari, digunakan untuk melakukan validasi. 
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Scan ulang jari");
    startTime = millis();
    // proses re pengambilan gambar
    while (finger.getImage() != FINGERPRINT_OK) {
      if (millis() - startTime > timeoutDuration) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Timeout! Ulangi");
        delay(3000);
        return;
      }
    }
    // proses re convert ke template
    if (finger.image2Tz(2) != FINGERPRINT_OK) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Gagal ubah img2");
      delay(1000);
      continue;
    }

    // Cocokkan keduanya
    // bandingkan template 1 & 2 menggunakan fungsi .createModel(), jika 2 template tidak cocok maka continue.
    // code setelah continue tidak akan dieksekusi & ulangi kode dari awal.
    if (finger.createModel() != FINGERPRINT_OK) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Gagal cocokkan");
      delay(1000);
      continue;
    }

    // Simpan ke slot
    // simpan idFinger ke model storage & cek kondisi jika berhasil menyimpan idFinger
    if (finger.storeModel(getFingerID) == FINGERPRINT_OK) {
      finger.LEDcontrol(FINGERPRINT_LED_PURPLE, FINGERPRINT_LED_FLASHING, 25, 3); 
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Tersimpan ID ");
      lcd.print(getFingerID);
    } else {
      finger.LEDcontrol(FINGERPRINT_LED_BLUE, FINGERPRINT_LED_FLASHING, 25, 2); 
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Gagal simpan");
    }
    // beri delay & break while
    delay(2000);
    break;
  }

  // Step 5: Tunggu tombol untuk lanjut verifikasi
  /* disini agak triky
     -intinya while 1 akan bersifat true (melakukan looping) selama tombol tidak ditekan *btn bernilai high secara default
     -kemudian ketika ditekan, btn=low & while pertama menjadi false.  
     -diwhile 2 ketika tombol ditekan terus maka btn=low & terjadi looping selama btn tidak kembali ke kondisi default * HIGH.
     -while 2 false jika btn=high lagi*/
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tekan btn lanjut");
  while (digitalRead(btn) == HIGH) {
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Tekan tombol");
    lcd.setCursor(1, 1);
    lcd.print("untuk lanjut");
  }  
  while (digitalRead(btn) == LOW); 

  // Step 6: Verifikasi jari terus menerus
  while (true) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Tempel jari...");

    // while disini digunakan untuk berjalan terus sampai sensor menangkap gambar yang benar.
    while (finger.getImage() != FINGERPRINT_OK);

    // ubah sidik jari yang ditangkap & ubah menjadi template slot 1, jika gagal maka ulang dari while
    if (finger.image2Tz(1) != FINGERPRINT_OK) continue;
    // cek kondisi jika pencarian teplate pada slot 1 sesuai maka tampilkan "jari sesuai" & sebaliknya tampilkan "jari tidak cocok"
    if (finger.fingerSearch() == FINGERPRINT_OK) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Jari sesuai");
      delay(1000);
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Jari tidak cocok");
      delay(1000);
    }
    delay(2000);
  }
}

// fungsi untuk mengetest LCD
void testLcd() {
  // pengetesan backlight
  lcd.backlight();
  delay(500);
  lcd.noBacklight();
  delay(500);
  lcd.backlight();

  // kode dibawah digunakan untuk pengetesan penulisan
  // .setCursor digunakan untuk mengatur arah row & collumn awal tulisan
  // .clear digunakan untuk membersihkan layar serta mereset Cursor
  lcd.setCursor(20,3); 
  lcd.print("Starting System..");
  delay(800);
  lcd.clear();
  // 3
  lcd.setCursor(8,1);
  lcd.print("3");
  delay(800);
  lcd.clear();
  // 2
  lcd.setCursor(8,1);
  lcd.print("2");
  delay(800);
  lcd.clear();
  // 1
  lcd.setCursor(8,1);
  lcd.print("1");
  delay(800);
  lcd.clear();
}

// fungsi untuk mengetes Relay
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
}
// fungsi untuk mengetes keypad
void testKeypad() {
  char key = readKeypad();
  
  if (key != '\0') {
    Serial.print("Key pressed: ");
    Serial.println(key);

    // Tampilkan di LCD
    lcd.setCursor(0, 1);
    lcd.print("Pressed: ");
    lcd.print(key);
    lcd.print("    "); // Clear sisa karakter
  }
  
  delay(10);
}

// ==================================end of fungsi Debug==================================

void setup() {
  // untuk rfid
  // aktifkan serial komunikasi dengan baud rate sekian
  // aktifkan komunikasi SPI dengan melakukan Override remapping pin SCK, MISO, MOSI & SS dengan pin yang digunakan
  Serial.begin(115200);
  SPI.begin(18, 19, 23, 15);


  // untuk fingerprint
  // set baudRate, format data serial, serta pin rx & tx untuk mySerial
  // Warning!! pin rx & tx tidak serta merta support UART default, namun bisa remap selama tidak bentrok dengan fungsi lain
  // aktifkan finger yang telah diset dengan parameter baudRate 57600
  mySerial.begin(57600, SERIAL_8N1, 16, 17); // RX = 16, TX = 17
  finger.begin(57600);

  // inisiasi modul rfid
  rfid.PCD_Init();   

  // inisiasi lcd
  lcd.init();
  
  // Inisialisasi PCF8575
  // dengan mengecek apa pcf terkoneksi kemudian beri output error
  while (!pcf.begin()) {
    lcd.clear();
    lcd.setCursor(2, 1);
    lcd.print("PCF8575 Error!");
    delay(2000);
  }
  // Set semua pin kolom sebagai output HIGH (tidak aktif)
  /* buat perulangan for dengen kondisi awal c = 0, ketika c kurang dari COLS, maka c +1 
     maka write pcf pin di colPin index 0 sebagai high
     iterasi sampai c=cols*/
  for (byte c = 0; c < COLS; c++) {
    pcf.write(colPins[c], HIGH);
  }
  // Set semua pin baris sebagai input (HIGH karena pull-up)
  /* buat iterasi +1 r sampai r = ROWS
  atur rowsPin[index-r] HIGH*/
  for (byte r = 0; r < ROWS; r++) {
    pcf.write(rowPins[r], HIGH);
  }

  // inisiasi pin relay sebagai output
  pinMode(relayMain, OUTPUT);
  pinMode(relaySecond, OUTPUT);
  digitalWrite(relayMain, HIGH);
  digitalWrite(relaySecond, HIGH);

  // inisiasi btn sebagai input_pullup
  pinMode(btn, INPUT_PULLUP); 

  // testing output
  testLcd();
  // testRelay();

  // dev mode
  // finger.begin(57600);
  // finger.verifyPassword();
  // finger.emptyDatabase(); // hanya saat awal boot
}

void loop() {
  // ==================================pemanggilan debug input==================================

  // testBtn();
  // cetakRfid();
  // testFinger();
  // testKeypad();
}

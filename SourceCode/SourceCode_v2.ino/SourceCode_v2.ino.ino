#include <WiFi.h>
#include <WiFiManager.h>
// #include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Fingerprint.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <TimeLib.h>
#include <Preferences.h>

// Definisi Pin
// definisikan pin untuk solenoid 1 *buka pintu & solenoid 2 *kunci pintu
// definisikan pin untuk tombol fisik
// definisikan pin reset untuk rfid
// definisikan SS untuk select RFID
#define SOLENOID_BUKA_PIN 18   // GPIO18 untuk buka pintu
#define SOLENOID_KUNCI_PIN 19   // GPIO19 untuk kunci pintu
#define TOMBOL_PIN 23          // GPIO23 untuk tombol fisik
#define RST_PIN 5              // GPIO5 untuk reset RFID
#define SS_PIN 4               // GPIO4 untuk chip select RFID

// Konfigurasi Keypad
// inisiasi rows & col dengan tipe daya byte dengan nilai 4 untuk rows & 3 untuk cols.
// buat array 2D keys dengan jumlah rows x col, 4 baris x 3 kolom, dengan isi, 1, 2, 3, ...#
// deklarasikan array rowsPin dengan ukuran rows (4) dengan 4 pin GPIO untuk baris 
// deklarasikan array colPin dengan ukuran col (3) dengan 3 pin GPIO untuk kolom
// Membuat objek keypad dari class Keypad milik library Keypad.h, dengan parameter: hasil pemetaan tombol (makeKeymap(keys)), pin baris (rowPins), pin kolom (colPins), dan jumlah baris-kolom)
const byte BARIS = 4;
const byte KOLOM = 3;
char tombol[BARIS][KOLOM] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte pinBaris[BARIS] = {13, 12, 14, 27};
byte pinKolom[KOLOM] = {26, 25, 33};
Keypad keypad = Keypad(makeKeymap(tombol), pinBaris, pinKolom, BARIS, KOLOM);

// Setup LCD
// dengan library liquidCrystal, dengan parameter berupa (alamat i2c, kolom & baris)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Setup Fingerprint
// inisiasi object fingerSerial dari class HardwareSerial dengan parameter berupa pin yang akan dipakai *disini memakai GPIO 2
// kemudian inisiasi finger dari class milik library adafruit_FingerPrint, dengan parameter(&fingerSerial) *berupa pin yang dipakai oleh fingerSerial yang telah dipakai sebelumnya
HardwareSerial mySerial(2);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// Setup RFID
// inisiasi object mfrc522  dari class MFRC522 milik library MFRC522 dengan parameter berupa ss pin & rst pin sebelumnya
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Setup Blynk
// set auth blynk
// set ssid & password wifi
char auth[] = "KODE_BLYNK_ANDA";
char ssid[] = "NAMA_WIFI_ANDA";
char pass[] = "PASSWORD_WIFI_ANDA";

// Penyimpanan preferensi
// inisiasi class Preferences digunakan untuk menyimpan data secara permanen
Preferences preferences;

// Variabel
String sandiAdmin = "123456";
String sandiPintu = "0000";
bool modeMalam = false;
int mulaiMalam = 16; // Jam 16:00 (4 PM)
int akhirMalam = 4;  // Jam 4:00 (4 AM)
bool diModeSetting = false;
bool diSubMenu = false;
int menuSekarang = 0;
int subMenuSekarang = 0;
bool wifiBerubah = false;

void setup() {
  //atur Serial ke 115200 baud rate / kecepatan komunikasi serial ke komputer *bit
  Serial.begin(115200);
  
  // Inisialisasi komponen sebagai OUTPUT & INPUT_PULLUP
  pinMode(SOLENOID_BUKA_PIN, OUTPUT);
  pinMode(SOLENOID_KUNCI_PIN, OUTPUT);
  pinMode(TOMBOL_PIN, INPUT_PULLUP);
  
  // Inisialisasi LCD dengan melakukan komunikasi ke LCD, nyalakan backlight
  lcd.init();
  lcd.backlight();
  
  // Inisialisasi RFID
  // inisiasi SPI dari library SPI.H untuk memulai koneksi SPI (Serial Peripheral Interface) untuk menghubungkan sensor
  // inisiasi object mftc522 dari class milik library mfrc522.
  SPI.begin();
  mfrc522.PCD_Init();
  
  // mulai object sensor "finger" dari library Adafruit_Fingerprint dengan baud rate 57600
  finger.begin(57600);
  
  // Inisialisasi preferensi
  preferences.begin("pintu-otomatis", false);
  muatPengaturan();
  
  // Hubungkan WiFi dan Blynk
  setupWiFi();
  
  // Set kondisi awal pintu
  digitalWrite(SOLENOID_BUKA_PIN, LOW);
  digitalWrite(SOLENOID_KUNCI_PIN, HIGH);
  
  // Tampilan awal
  tampilkanWaktuTanggal();
}

void loop() {
  static unsigned long terakhirUpdate = 0;
  static unsigned long terakhirSync = 0;
  
  // Sinkronisasi waktu setiap jam
  if (millis() - terakhirSync > 3600000) {
    sinkronisasiWaktu();
    terakhirSync = millis();
  }
  
  // Update tampilan setiap detik
  if (millis() - terakhirUpdate > 1000) {
    tampilkanWaktuTanggal();
    terakhirUpdate = millis();
    
    // Cek mode malam
    cekModeMalam();
  }
  
  // Handle Blynk
  // Blynk.run();
  
  // Handle tombol fisik
  if (digitalRead(TOMBOL_PIN) == LOW) {
    delay(50); // debounce
    if (digitalRead(TOMBOL_PIN) == LOW) {
      handleTombol();
    }
  }
  
  // Handle input keypad
  char tombol = keypad.getKey();
  if (tombol) {
    handleInputKeypad(tombol);
  }
  
  // Handle fingerprint
  if (finger.getImage() == FINGERPRINT_OK) {
    handleFingerprint();
  }
  
  // Handle RFID
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    handleRFID();
  }
}

void setupWiFi() {
  WiFiManager wifiManager;
  
  // Untuk reset pengaturan WiFi, hapus komentar berikut
  // wifiManager.resetSettings();
  
  wifiManager.setTimeout(180);
  
  if (!wifiManager.autoConnect("SmartDoorLock")) {
    Serial.println("Gagal terhubung ke WiFi");
    delay(3000);
    ESP.restart();
  }
  
  Serial.println("Terhubung ke WiFi");
  // Blynk.config(auth);
  // while (Blynk.connect() == false) {
  //   // Tunggu sampai terhubung
  // }
}

void muatPengaturan() {
  sandiAdmin = preferences.getString("sandiAdmin", "123456");
  sandiPintu = preferences.getString("sandiPintu", "0000");
  mulaiMalam = preferences.getInt("mulaiMalam", 16);
  akhirMalam = preferences.getInt("akhirMalam", 4);
  
  // Untuk data RFID dan fingerprint bisa ditambahkan
}

void simpanPengaturan() {
  preferences.putString("sandiAdmin", sandiAdmin);
  preferences.putString("sandiPintu", sandiPintu);
  preferences.putInt("mulaiMalam", mulaiMalam);
  preferences.putInt("akhirMalam", akhirMalam);
}

void tampilkanWaktuTanggal() {
  // Implementasi tampilan waktu dan tanggal
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Jam: ");
  lcd.print(hour());
  lcd.print(":");
  if (minute() < 10) lcd.print("0");
  lcd.print(minute());
  
  lcd.setCursor(0, 1);
  lcd.print("Tgl: ");
  lcd.print(day());
  lcd.print("/");
  lcd.print(month());
}

void cekModeMalam() {
  int jamSekarang = hour();
  modeMalam = (jamSekarang >= mulaiMalam || jamSekarang < akhirMalam);
  
  if (modeMalam) {
    digitalWrite(SOLENOID_KUNCI_PIN, LOW); // Matikan kunci
  } else {
    digitalWrite(SOLENOID_KUNCI_PIN, HIGH); // Nyalakan kunci
  }
}

void handleTombol() {
  if (!diModeSetting) {
    // Buka menu setting jika menekan tombol saat tidak di mode setting
    mintaSandiAdmin();
  } else {
    // Keluar dari mode setting
    diModeSetting = false;
    diSubMenu = false;
    lcd.clear();
    tampilkanWaktuTanggal();
  }
}

void mintaSandiAdmin() {
  lcd.clear();
  lcd.print("Masukkan Sandi Admin:");
  String input = bacaInputKeypad();
  
  if (input == sandiAdmin) {
    diModeSetting = true;
    tampilkanMenuUtama();
  } else {
    lcd.clear();
    lcd.print("Sandi Salah!");
    delay(2000);
    tampilkanWaktuTanggal();
  }
}

String bacaInputKeypad() {
  String input = "";
  lcd.setCursor(0, 1);
  
  while (true) {
    char tombol = keypad.getKey();
    
    if (tombol) {
      if (tombol == '#') {
        return input;
      } else if (tombol == '*') {
        return ""; // Batal
      } else if (tombol >= '0' && tombol <= '9') {
        input += tombol;
        lcd.print("*");
      }
    }
  }
}

void tampilkanMenuUtama() {
  lcd.clear();
  switch (menuSekarang) {
    case 0:
      lcd.print("1. Kelola Fingerprint");
      lcd.setCursor(0, 1);
      lcd.print("2. Kelola RFID");
      break;
    case 1:
      lcd.print("3. Ganti Sandi Pintu");
      lcd.setCursor(0, 1);
      lcd.print("4. Mode Malam");
      break;
    case 2:
      lcd.print("5. Pengaturan WiFi");
      lcd.setCursor(0, 1);
      lcd.print("* Kembali");
      break;
  }
}

void handleInputKeypad(char tombol) {
  if (!diModeSetting) {
    // Handle input untuk membuka pintu
    handleInputPintu(tombol);
  } else {
    // Handle input di mode setting
    handleInputSetting(tombol);
  }
}

void handleInputPintu(char tombol) {
  static String input = "";
  
  if (tombol == '#') {
    if (input == sandiPintu) {
      bukaPintu();
    } else {
      lcd.clear();
      lcd.print("Sandi Salah!");
      delay(2000);
      tampilkanWaktuTanggal();
    }
    input = "";
  } else if (tombol == '*') {
    input = "";
  } else {
    input += tombol;
  }
}

void bukaPintu() {
  digitalWrite(SOLENOID_BUKA_PIN, HIGH);
  lcd.clear();
  lcd.print("Pintu Terbuka!");
  delay(3000);
  digitalWrite(SOLENOID_BUKA_PIN, LOW);
  tampilkanWaktuTanggal();
  
  // Jika mode malam, aktifkan kedua solenoid
  if (modeMalam) {
    digitalWrite(SOLENOID_KUNCI_PIN, HIGH);
    delay(3000);
    digitalWrite(SOLENOID_KUNCI_PIN, LOW);
  }
}

void handleInputSetting(char tombol) {
  if (!diSubMenu) {
    // Navigasi menu utama
    if (tombol == '1') {
      menuSekarang = 0;
      tampilkanSubMenuFingerprint();
    } else if (tombol == '2') {
      menuSekarang = 0;
      tampilkanSubMenuRFID();
    } else if (tombol == '3') {
      menuSekarang = 1;
      gantiSandiPintu();
    } else if (tombol == '4') {
      menuSekarang = 1;
      aturModeMalam();
    } else if (tombol == '5') {
      menuSekarang = 2;
      aturWiFi();
    } else if (tombol == '*') {
      diModeSetting = false;
      tampilkanWaktuTanggal();
    } else if (tombol == '#' || tombol == '6') {
      menuSekarang = (menuSekarang + 1) % 3;
      tampilkanMenuUtama();
    }
  } else {
    // Handle sub menu
    if (tombol == '*') {
      diSubMenu = false;
      tampilkanMenuUtama();
    }
    // Implementasi sub menu sesuai kebutuhan
  }
}

// =============================================
// FUNGSI SUB MENU FINGERPRINT
// =============================================
void tampilkanSubMenuFingerprint() {
  diSubMenu = true;
  lcd.clear();
  lcd.print("1.Tambah Fingerprint");
  lcd.setCursor(0, 1);
  lcd.print("2.Hapus Fingerprint");
}

void handleSubMenuFingerprint(char pilihan) {
  if (pilihan == '1') {
    tambahFingerprint();
  } else if (pilihan == '2') {
    hapusFingerprint();
  }
}

void tambahFingerprint() {
  lcd.clear();
  lcd.print("Tempelkan jari...");
  
  int id = 0;
  while (id == 0) {
    lcd.setCursor(0, 1);
    lcd.print("ID (1-127):");
    String input = bacaInputKeypad();
    id = input.toInt();
    if (id < 1 || id > 127) {
      lcd.clear();
      lcd.print("ID tidak valid!");
      delay(2000);
      id = 0;
      lcd.clear();
      lcd.print("Tempelkan jari...");
    }
  }

  int p = -1;
  lcd.clear();
  lcd.print("Tempelkan jari 3x");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_OK) {
      p = finger.image2Tz(1);
      if (p == FINGERPRINT_OK) {
        lcd.clear();
        lcd.print("Angkat jari");
        delay(2000);
        p = finger.getImage();
        if (p == FINGERPRINT_OK) {
          p = finger.image2Tz(2);
          if (p == FINGERPRINT_OK) {
            p = finger.createModel();
            if (p == FINGERPRINT_OK) {
              p = finger.storeModel(id);
              if (p == FINGERPRINT_OK) {
                lcd.clear();
                lcd.print("Berhasil disimpan!");
                delay(2000);
                return;
              }
            }
          }
        }
      }
    }
    delay(500);
  }
  lcd.clear();
  lcd.print("Gagal menyimpan!");
  delay(2000);
}

void hapusFingerprint() {
  lcd.clear();
  lcd.print("Masukkan ID:");
  lcd.setCursor(0, 1);
  String input = bacaInputKeypad();
  int id = input.toInt();
  
  if (id < 1 || id > 127) {
    lcd.clear();
    lcd.print("ID tidak valid!");
    delay(2000);
    return;
  }
  
  if (finger.deleteModel(id) == FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("ID ");
    lcd.print(id);
    lcd.print(" dihapus");
    delay(2000);
  } else {
    lcd.clear();
    lcd.print("Gagal menghapus!");
    delay(2000);
  }
}

// =============================================
// FUNGSI SUB MENU RFID
// =============================================
void tampilkanSubMenuRFID() {
  diSubMenu = true;
  lcd.clear();
  lcd.print("1.Tambah RFID");
  lcd.setCursor(0, 1);
  lcd.print("2.Hapus RFID");
}

void handleSubMenuRFID(char pilihan) {
  if (pilihan == '1') {
    tambahRFID();
  } else if (pilihan == '2') {
    hapusRFID();
  }
}

void tambahRFID() {
  lcd.clear();
  lcd.print("Tempelkan kartu...");
  
  while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    // Tunggu sampai kartu terdeteksi
    if (keypad.getKey() == '*') return; // Batalkan jika tekan *
  }
  
  String uid = bacaUIDRFID();
  preferences.putString(("rfid_" + uid).c_str(), "1");
  
  lcd.clear();
  lcd.print("RFID ditambahkan");
  lcd.setCursor(0, 1);
  lcd.print("UID: " + uid.substring(0, 8));
  delay(3000);
}

void hapusRFID() {
  lcd.clear();
  lcd.print("Tempelkan kartu...");
  
  while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    // Tunggu sampai kartu terdeteksi
    if (keypad.getKey() == '*') return; // Batalkan jika tekan *
  }
  
  String uid = bacaUIDRFID();
  preferences.remove(("rfid_" + uid).c_str());
  
  lcd.clear();
  lcd.print("RFID dihapus");
  lcd.setCursor(0, 1);
  lcd.print("UID: " + uid.substring(0, 8));
  delay(3000);
}

String bacaUIDRFID() {
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  mfrc522.PICC_HaltA();
  return uid;
}

// =============================================
// FUNGSI GANTI SANDI PINTU
// =============================================
void gantiSandiPintu() {
  diSubMenu = true;
  lcd.clear();
  lcd.print("Masukkan sandi baru:");
  
  String sandiBaru = bacaInputKeypad();
  if (sandiBaru.length() < 4) {
    lcd.clear();
    lcd.print("Min 4 digit!");
    delay(2000);
    return;
  }
  
  sandiPintu = sandiBaru;
  simpanPengaturan();
  
  lcd.clear();
  lcd.print("Sandi diganti!");
  delay(2000);
}

// =============================================
// FUNGSI ATUR MODE MALAM
// =============================================
void aturModeMalam() {
  diSubMenu = true;
  lcd.clear();
  lcd.print("1.Ubah mulai malam");
  lcd.setCursor(0, 1);
  lcd.print("2.Ubah akhir malam");
  
  char pilihan = keypad.waitForKey();
  
  if (pilihan == '1') {
    ubahJamModeMalam(true); // Ubah jam mulai
  } else if (pilihan == '2') {
    ubahJamModeMalam(false); // Ubah jam akhir
  }
}

void ubahJamModeMalam(bool isMulai) {
  lcd.clear();
  lcd.print(isMulai ? "Jam mulai (0-23):" : "Jam akhir (0-23):");
  
  String input = bacaInputKeypad();
  int jam = input.toInt();
  
  if (jam < 0 || jam > 23) {
    lcd.clear();
    lcd.print("Jam tidak valid!");
    delay(2000);
    return;
  }
  
  if (isMulai) {
    mulaiMalam = jam;
  } else {
    akhirMalam = jam;
  }
  
  simpanPengaturan();
  
  lcd.clear();
  lcd.print(isMulai ? "Mulai diubah: " : "Akhir diubah: ");
  lcd.setCursor(0, 1);
  lcd.print(jam);
  lcd.print(":00");
  delay(2000);
}

// =============================================
// FUNGSI ATUR WiFi
// =============================================
void aturWiFi() {
  diSubMenu = true;
  lcd.clear();
  lcd.print("1.Reset WiFi");
  lcd.setCursor(0, 1);
  lcd.print("2.Ubah token Blynk");
  
  char pilihan = keypad.waitForKey();
  
  if (pilihan == '1') {
    resetWiFi();
  } else if (pilihan == '2') {
    // ubahTokenBlynk();
  }
}

void resetWiFi() {
  lcd.clear();
  lcd.print("Reset WiFi...");
  delay(1000);
  
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  ESP.restart();
}

// void ubahTokenBlynk() {
//   lcd.clear();
//   lcd.print("Token Blynk baru:");
  
//   String tokenBaru = bacaInputKeypad();
//   if (tokenBaru.length() == 0) {
//     lcd.clear();
//     lcd.print("Dibatalkan!");
//     delay(2000);
//     return;
//   }
  
//   preferences.putString("blynkToken", tokenBaru);
//   // auth = tokenBaru.c_str();
  
//   lcd.clear();
//   lcd.print("Token diubah!");
//   delay(2000);
//   // Blynk.config(auth);
// }

// =============================================
// FUNGSI HANDLE FINGERPRINT
// =============================================
void handleFingerprint() {
  if (finger.image2Tz() != FINGERPRINT_OK) return;
  
  if (finger.fingerFastSearch() == FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Sidik jari cocok!");
    lcd.setCursor(0, 1);
    lcd.print("ID: ");
    lcd.print(finger.fingerID);
    bukaPintu();
  } else {
    lcd.clear();
    lcd.print("Sidik jari tidak dikenal!");
    delay(2000);
    tampilkanWaktuTanggal();
  }
}

// =============================================
// FUNGSI HANDLE RFID
// =============================================
void handleRFID() {
  String uid = bacaUIDRFID();
  
  if (preferences.getString(("rfid_" + uid).c_str(), "").equals("1")) {
    lcd.clear();
    lcd.print("RFID dikenali!");
    lcd.setCursor(0, 1);
    lcd.print("UID: " + uid.substring(0, 8));
    bukaPintu();
  } else {
    lcd.clear();
    lcd.print("RFID tidak dikenal!");
    delay(2000);
    tampilkanWaktuTanggal();
  }
}

// =============================================
// FUNGSI SINKRONISASI WAKTU
// =============================================
void sinkronisasiWaktu() {
  // Jika tidak ada RTC, gunakan waktu dari NTP server
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    setTime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, 
            timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
  }
}
// Tambahkan juga fungsi Blynk untuk kontrol dari aplikasi
// BLYNK_WRITE(V0) {
//   int nilai = param.asInt();
//   if (nilai == 1) {
//     bukaPintu();
//   }
// }
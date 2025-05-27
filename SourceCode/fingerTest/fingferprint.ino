#include <Adafruit_Fingerprint.h>

// inisiasi mySerial dengan hardwareSerial ke-2 yaitu (UART2) pin GPIO 16 & GPIO 17 pada esp32
HardwareSerial mySerial(2);

// inisiasi object finger dengan class Adafruit_Fingerprint dengan parameter berupa semua pin yang diset mySerial
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);


const int btn = 25;
int btnState = 0;
int lastBtnState = 0;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;  // waktu debounce (dalam ms)


void setup() {
  Serial.begin(115200);   // Serial monitor
  delay(1000);
  
  // Mulai komunikasi dengan sensor fingerprint
  mySerial.begin(57600, SERIAL_8N1, 16, 17); // RX = 16, TX = 17
  finger.begin(57600);

  // Cek apakah sensor terdeteksi
  if (finger.verifyPassword()) {
    Serial.println("Sensor fingerprint terdeteksi!");
  } else {
    Serial.println("Gagal mendeteksi sensor fingerprint :(");
    while (1) { delay(1); }
  }

  finger.getTemplateCount();
  Serial.print("Jumlah sidik jari tersimpan: "); Serial.println(finger.templateCount);
}

void loop() {
  getFingerprintID();
  delay(1000); // Delay 1 detik sebelum scan berikutnya
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Gambar sidik jari diambil");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("Tidak ada jari");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Kesalahan komunikasi");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Gagal mengambil gambar");
      return p;
    default:
      Serial.println("Kesalahan tidak diketahui");
      return p;
  }

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Gambar diubah ke template");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Gambar sidik jari jelek");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Kesalahan komunikasi");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Gagal mengenali fitur sidik jari");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Gambar tidak valid");
      return p;
    default:
      Serial.println("Kesalahan tidak diketahui");
      return p;
  }

  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Sidik jari cocok ditemukan!");
    Serial.print("ID: "); Serial.print(finger.fingerID);
    Serial.print(" dengan confidence: "); Serial.println(finger.confidence);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Kesalahan komunikasi");
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Sidik jari tidak ditemukan");
  } else {
    Serial.println("Kesalahan tidak diketahui");
  }

  return p;
}

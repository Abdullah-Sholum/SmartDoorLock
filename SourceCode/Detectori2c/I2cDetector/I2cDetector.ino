#include <Wire.h>

void scanI2CDevices() {
  byte error, address;
  int count = 0;

  Serial.println("Memulai scan I2C...");
  Serial.println("Scanning alamat I2C (0x03 - 0x77):");

  for (address = 0x03; address <= 0x77; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device ditemukan di alamat 0x");
      if (address < 0x10)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  --> Kemungkinan PCF8574 / PCF8575");

      count++;
      delay(10); // Delay ringan untuk kestabilan
    } else if (error == 4) {
      Serial.print("Kesalahan tidak diketahui di alamat 0x");
      if (address < 0x10)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  if (count == 0)
    Serial.println("Tidak ditemukan perangkat I2C.");
  else
    Serial.println("Scan selesai.");
}

void setup() {
  Wire.begin();
  Serial.begin(9600);
  delay(1000); // Tunggu serial aktif
  scanI2CDevices(); // Panggil saat setup
}

void loop() {
  // Kosong atau dapat tambahkan perulangan scan jika dibutuhkan
}

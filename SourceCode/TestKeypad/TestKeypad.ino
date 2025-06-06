#include <Wire.h>
#include <PCF8575.h>
#include <LiquidCrystal_I2C.h>

// Inisialisasi LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Alamat I2C LCD 0x27, ukuran 16x2

// Inisialisasi PCF8575 di alamat 0x20
PCF8575 pcf(0x20);

// Keypad 4x3
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

void setup() {
  Wire.begin();
  Serial.begin(115200);

  // Inisialisasi LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Keypad Test");
  lcd.setCursor(0, 1);
  lcd.print("Press any key...");

  // Inisialisasi PCF8575
  if (!pcf.begin()) {
    Serial.println("PCF8575 not found!");
    lcd.clear();
    lcd.print("PCF8575 Error!");
    while(1);
  }

  // Set semua pin kolom sebagai output HIGH (tidak aktif)
  for (byte c = 0; c < COLS; c++) {
    pcf.write(colPins[c], HIGH);
  }

  // Set semua pin baris sebagai input (HIGH karena pull-up)
  for (byte r = 0; r < ROWS; r++) {
    pcf.write(rowPins[r], HIGH);
  }

  Serial.println("System ready");
}

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

// Fungsi untuk membaca keypad
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

// wtffff

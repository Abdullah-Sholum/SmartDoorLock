#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PCF8575.h>
#include <Keypad.h>

// Pin dan alamat
#define SDA_PIN 21
#define SCL_PIN 22
#define Buzz     7
#define Btn 10
#define PCF_ADDR 0x20
#define LCD_ADDR 0x27

PCF8575 pcf(PCF_ADDR);
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// Keypad
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {0, 1, 2, 3}; // P0–P3
byte colPins[COLS] = {4, 5, 6};    // P4–P6
 m
class Keypad_PCF : public Keypad {
  public:
    Keypad_PCF(char *userKeymap, byte *row, byte *col, byte numRows, byte numCols)
    : Keypad(userKeymap, row, col, numRows, numCols) {}

    virtual void pin_mode(byte pinNum, byte mode) {}
    virtual void pin_write(byte pinNum, boolean level) {
      pcf.write(pinNum, level);
    }
    virtual int pin_read(byte pinNum) {
      return pcf.read(pinNum);
    }
};

Keypad_PCF keypad((char*)keys, rowPins, colPins, ROWS, COLS);

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  // Inisialisasi PCF8575 dulu SEBELUM digunakan
  pcf.begin();
  delay(100);

  // Set semua output dalam keadaan HIGH (relay OFF)
  pcf.write(Buzz, HIGH);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Memulai Sistem...");
  delay(1000);
  lcd.clear();

  Serial.println("Sistem siap.");
}

void loop() {
  char key = keypad.getKey();
  if (key) {
    Serial.print("Tombol: ");
    Serial.println(key);
    lcd.clear();
    lcd.print("Tombol:");
    lcd.setCursor(0, 1);
    lcd.print(key);
  }

  if (key == '1') {
    Serial.println("Relay 1 ON");
    pcf.write(Buzz, LOW);  // Aktifkan
    delay(500);

  }

  if (key == '2') {
    Serial.println("Relay 2 ON");
    pcf.write(Buzz, LOW);
    delay(200);
  }

  if (key == '#') {
    pcf.write(Buzz, LOW);
    delay(1000);
  }if (pcf.read(Btn) == LOW) {
    Serial.println("Tombol ditekan");
    pcf.write(Buzz, LOW);
    delay(2000);
  }
}

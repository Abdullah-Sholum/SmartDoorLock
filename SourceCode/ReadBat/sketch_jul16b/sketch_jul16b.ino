#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define BMS_PIN 34       // Pin ADC ESP32
#define R_DIVIDER 4.0    // Karena pakai 4 resistor 10k
#define I2C_ADDR 0x27    // Alamat I2C umum, ganti jika LCD-mu berbeda

LiquidCrystal_I2C lcd(I2C_ADDR, 16, 2);

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);  // 0-4095
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("BMS 3S Monitor");
  delay(2000);
  lcd.clear();
}

void loop() {
  int raw = analogRead(BMS_PIN);
  float voltage_adc = (raw / 4095.0) * 3;
  float voltage_battery = voltage_adc * R_DIVIDER;
  int battery_percent = mapBatteryToPercent(voltage_battery);

  // Tampilkan di Serial Monitor
  Serial.print("ADC: ");
  Serial.print(raw);
  Serial.print(" | Tegangan: ");
  Serial.print(voltage_battery, 2);
  Serial.print("V | Persen: ");
  Serial.print(battery_percent);
  Serial.println("%");

  // Tampilkan di LCD
  lcd.setCursor(0, 0);
  lcd.print("Volt: ");
  lcd.print(voltage_battery, 2);
  lcd.print("V   ");

  lcd.setCursor(0, 1);
  lcd.print("Persen: ");
  lcd.print(battery_percent);
  lcd.print("%   ");

  delay(1000);
}

int mapBatteryToPercent(float voltage) {
  if (voltage >= 12.6) return 100;
  else if (voltage <= 9.0) return 0;
  else return (int)(((voltage - 9.0) / (12.6 - 9.0)) * 100);
}
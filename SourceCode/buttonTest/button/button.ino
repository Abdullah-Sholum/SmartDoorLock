const int btn = 25;
int btnState = 0;
int lastBtnState = 0;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;  // waktu debounce (dalam ms)

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(btn, INPUT); 
}

void loop() {
  int reading = digitalRead(btn);

  // Cek apakah status tombol berubah
  if (reading != lastBtnState) {
    lastDebounceTime = millis();  // reset waktu debounce
  }

  // Hanya jika sudah melewati waktu debounce, status tombol dianggap valid
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // Hanya update jika status tombol benar-benar berubah
    if (reading != btnState) {
      btnState = reading;

      if (btnState == HIGH) {
        Serial.println("padam abangku");
      } else {
        Serial.println("menyala abangku");
      }
    }
  }

  // Simpan status terakhir untuk dibandingkan nanti
  lastBtnState = reading;

  // Delay agar tidak terlalu sering mencetak output
  delay(500);

}

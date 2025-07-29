/*Disini merupkan kode untuk reset flash arduino*/

#include <Arduino.h>
#include <nvs.h>
#include <nvs_flash.h>

void factoryReset() {
  nvs_flash_erase();  // Hapus semua isi NVS (Preferences, dsb)
  ESP.restart();      // Restart ESP
}

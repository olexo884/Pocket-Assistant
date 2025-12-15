#include <EEPROM.h>
#include "app/app_state.h"
#include "eeprom.h"

void saveToEEPROM() {
  // Збереження даних часу
  EEPROM.write(0, currentHour);
  EEPROM.write(1, currentMinute);
  EEPROM.write(2, currentSecond);
  EEPROM.write(3, currentDay);
  EEPROM.write(4, currentMonth);
  EEPROM.write(5, currentYear - 2000);

  // Збереження SSID та пароля
  for (int i = 0; i < 32; i++) {
    EEPROM.write(6 + i, ssid[i]);
    if (ssid[i] == '\0') break;
  }

  for (int i = 0; i < 32; i++) {
    EEPROM.write(38 + i, pass[i]);
    if (pass[i] == '\0') break;
  }

  // Збереження значень modelAIIndex і tokens
  EEPROM.write(70, modelAIIndex);  // Місце для modelAIIndex
  EEPROM.put(71, tokens);          // Місце для tokens

  Serial.println("EEPROM save");
  EEPROM.commit();
}
void loadFromEEPROM() {
  // Завантаження даних часу
  currentHour = EEPROM.read(0);
  currentMinute = EEPROM.read(1);
  currentSecond = EEPROM.read(2);
  currentDay = EEPROM.read(3);
  currentMonth = EEPROM.read(4);
  currentYear = EEPROM.read(5) + 2000;

  // Завантаження SSID і пароля
  static char storedSSID[33];
  static char storedPass[33];

  for (int i = 0; i < 32; i++) {
    storedSSID[i] = EEPROM.read(6 + i);
    if (storedSSID[i] == '\0') break;
  }
  storedSSID[32] = '\0';

  for (int i = 0; i < 32; i++) {
    storedPass[i] = EEPROM.read(38 + i);
    if (storedPass[i] == '\0') break;
  }
  storedPass[32] = '\0';

  ssid = storedSSID;
  pass = storedPass;

  // Завантаження значень modelAIIndex і tokens
  modelAIIndex = EEPROM.read(70);   // Завантаження modelAIIndex
  tokens = EEPROM.get(71, tokens);  // Завантаження tokens

  Serial.println("EEPROM load");
}

#include <SPI.h>
#include <MFRC522.h>

#include "status_led.h"
#include "rfid_helpers.h"

#define SS_PIN 10
#define RST_PIN 9

MFRC522 mfrc522(SS_PIN, RST_PIN);
StatusLED stled;

byte cardUID[4];
bool cardRead = false;

#define NR_KNOWN_KEYS   2
byte knownKeys[NR_KNOWN_KEYS][MFRC522::MF_KEY_SIZE] =  {
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}, // Old cards
    {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5}  // New card
};

byte buffer[18];
byte cardData[15][16] = {0};

MFRC522::StatusCode status;
MFRC522::MIFARE_Key key;

bool authenticate(MFRC522::MIFARE_Key *key, byte block) {
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, key, &(mfrc522.uid));
  Serial.println(status);
  return (status == MFRC522::STATUS_OK);
}

bool readBlock(byte block, byte *buffer) {
  byte size = 18;
  status = mfrc522.MIFARE_Read(block, buffer, &size);
  Serial.println(status);
  return (status == MFRC522::STATUS_OK);
}

bool writeBlock(byte block, byte *buffer) {
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  Serial.println(status);
  return (status == MFRC522::STATUS_OK);
}

void dumpCardData(byte data[][16]) {
    Serial.println("Считанные данные:");
    for (int sector = 0; sector < 15; sector++) {
        Serial.print("Сектор "); Serial.print(sector); Serial.print(": ");
        for (int i = 0; i < 16; i++) {
            Serial.print(data[sector][i], HEX); Serial.print(" ");
        }
        Serial.println();
    }
}

bool readCard() {
  stled.set_state_fadnes(255, 165, 0, 20); // Оранжевое мигание
  Serial.println("[INFO] Ожидание карты...");
  while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    stled.tick();
  }

  stled.set_state(0, 128, 255, 500);

  Serial.println("[INFO] Карта найдена!");
  byte uid_size = min(mfrc522.uid.size, 4);
  memcpy(cardUID, mfrc522.uid.uidByte, uid_size);
  
  Serial.print("[INFO] UID карты: ");
  for (int i; i<4; i++){
    Serial.print(cardUID[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  bool success = false;
  for (byte sector = 0; sector < 15; sector++) {
    for (byte k = 0; k < NR_KNOWN_KEYS; k++) {
      memcpy(key.keyByte, knownKeys[k], MFRC522::MF_KEY_SIZE);
      if (authenticate(&key, sector * 4)) {
        if (readBlock(sector * 4, cardData[sector])) {
          Serial.print("[INFO] Сектор "); Serial.print(sector); Serial.println(" считан.");
          success = true;
          break;
        }
      }
    }
    if (!success) {
      Serial.println("[ERROR] Ошибка чтения");
      stled.set_state(255, 0, 0, 500);
      uint32_t start = millis();
      while (millis()-start<5000){
        stled.tick();
      }
      return false;
    }
  }

  // wait until no card
  Serial.println("[INFO] Ждём пока карту уберут ");
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  stled.set_state(255, 50, 0, 400);
  while ( true ) {
      byte buffer[4];
      byte bufferSize = sizeof(buffer);
      
      MFRC522::StatusCode status = mfrc522.PICC_WakeupA(buffer, &bufferSize);

      if (status != MFRC522::STATUS_OK) {
         break;
      }
      mfrc522.PICC_HaltA();
      stled.tick();
  }
  Serial.println("[INFO] Дождались карту уберут ");

  delay(500);
  digitalWrite(RST_PIN, HIGH);          // Сбрасываем модуль
  delayMicroseconds(2);                 // Ждем 2 мкс
  digitalWrite(RST_PIN, LOW);           // Отпускаем сброс
  mfrc522.PCD_Init();    
  return true;
}

bool writeUID() {
  Serial.println("[INFO] Ожидание новой карты...");
  while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
      stled.tick();
  }

  Serial.println("[INFO] Запись UID...");

  if (!mfrc522.MIFARE_SetUid(cardUID, (byte)4, true)) {
      Serial.println("[ERROR] Ошибка записи UID ");
      stled.set_state(255, 0, 0, 500);
      uint32_t start = millis();
      while (millis()-start<5000){
        stled.tick();
      }
      return false;
  }
  Serial.println("[INFO] UID успешно записан!");

  bool success = false;
  for (byte sector = 0; sector < 15; sector++) {
      for (byte k = 0; k < NR_KNOWN_KEYS; k++) {
          memcpy(key.keyByte, knownKeys[k], MFRC522::MF_KEY_SIZE);
          if (authenticate(&key, sector * 4)) {
            Serial.print("[INFO] Authentication successful for sector "); Serial.print(sector);
            if (writeBlock(sector * 4, cardData[sector])) {
              Serial.print("[INFO] Sector "); Serial.print(sector); Serial.println(" written successfully.");
              success = true;
              break;
            } else {
              Serial.print("[ERROR] Failed to write sector "); Serial.print(sector);
            }
          } else {
            Serial.print("[ERROR] Authentication failed for sector "); Serial.print(sector);
          }
      }
      if (!success) {
        Serial.println(sector);
        Serial.println("[ERROR] Ошибка Записи");
        stled.set_state(255, 0, 0, 500);
        uint32_t start = millis();
        while (millis()-start<5000){
          stled.tick();
        }
        return false;
      }
  }

  // wait until no card
  Serial.println("[INFO] Ждём пока карту уберут ");
  mfrc522.PICC_HaltA();
  stled.set_state(255, 50, 0, 400);
  while ( true ) {
      byte buffer[4];
      byte bufferSize = sizeof(buffer);
      
      MFRC522::StatusCode status = mfrc522.PICC_WakeupA(buffer, &bufferSize);

      if (status != MFRC522::STATUS_OK) {
        break;
      }
      mfrc522.PICC_HaltA();
      stled.tick();
  }
  Serial.println("[INFO] Дождались карту уберут ");

  delay(500);
  digitalWrite(RST_PIN, HIGH);          // Сбрасываем модуль
  delayMicroseconds(2);                 // Ждем 2 мкс
  digitalWrite(RST_PIN, LOW);           // Отпускаем сброс
  mfrc522.PCD_Init();
  return true;
}

void setup() {
    Serial.begin(115200);
    Serial.println("[INFO] Запуск инициализации!");
    SPI.begin();
    mfrc522.PCD_Init();
    stled.begin();

    Serial.println("[INFO] Система готова!");
    stled.set_state_fadnes(0, 255, 0, 20); // Зелёное мигание
}

void loop() {
    stled.tick();
    
    if (!cardRead) {
        if (readCard()) {
            cardRead = true;
            stled.set_state_fadnes(255, 255, 0, 18); // Жёлтый
        } else {
            stled.set_state(255, 0, 0); // Красный при ошибке
        }
    } else {
      cardRead = false;
      stled.set_state_fadnes(255, 0, 165, 18); // Фиолетовое мигание при записи
      writeUID();
    }
}
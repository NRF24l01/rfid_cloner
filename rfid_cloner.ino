#include <SPI.h>
#include <MFRC522.h>
#include "status_led.h"

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
byte block;
byte waarde[64][15];
MFRC522::MIFARE_Key originalKey;

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

  for (byte k = 0; k < NR_KNOWN_KEYS; k++) {
    memcpy(originalKey.keyByte, knownKeys[k], MFRC522::MF_KEY_SIZE);
    
    if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &originalKey, &(mfrc522.uid)) == MFRC522::STATUS_OK) {
      Serial.println("Authentication successful! Reading data...");
      for (block = 0; block < 64; block++) {
        byte size = 18;
        if (mfrc522.MIFARE_Read(block, buffer, &size) == MFRC522::STATUS_OK) {
          memcpy(waarde[block], buffer, 15);
        }
      }
      Serial.println("Data copied successfully. Now scan the second card to paste data.");
      break;
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

  if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &originalKey, &(mfrc522.uid)) != MFRC522::STATUS_OK) {
    Serial.println("Authentication failed on second card. Overwriting keys...");
    for (block = 3; block < 64; block += 4) {
      memcpy(buffer, originalKey.keyByte, MFRC522::MF_KEY_SIZE);
      buffer[6] = 0xFF; // Access bits (default)
      buffer[7] = 0x07;
      buffer[8] = 0x80;
      memcpy(&buffer[10], originalKey.keyByte, MFRC522::MF_KEY_SIZE);
      mfrc522.MIFARE_Write(block, buffer, 16);
    }
  }
    
    for (block = 0; block < 64; block++) {
      if (mfrc522.MIFARE_Write(block, waarde[block], 15) != MFRC522::STATUS_OK) {
        Serial.println("Write failed on block " + String(block));
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
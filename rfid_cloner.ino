#include <SPI.h>
#include <MFRC522.h>
#include "status_led.h"

#define SS_PIN 10
#define RST_PIN 9

MFRC522 mfrc522(SS_PIN, RST_PIN);
StatusLED stled;

byte cardUID[4];
bool cardRead = false;

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
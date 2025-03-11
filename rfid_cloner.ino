#include <SPI.h>
#include <MFRC522.h>
#include "status_led.h"

// Определение пинов
#define SS_PIN 10
#define RST_PIN 9

// Создание объектов
MFRC522 mfrc522(SS_PIN, RST_PIN);
StatusLED stled;

// Глобальные переменные
byte cardUID[4];
bool cardRead = false;

// Функция чтения карты
bool readCard() {
    Serial.println("[INFO] Ожидание карты...");
    while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        stled.tick();
    }

    Serial.println("[INFO] Карта найдена!");
    byte uid_size = min(mfrc522.uid.size, 4);
    memcpy(cardUID, mfrc522.uid.uidByte, uid_size);
    
    Serial.print("[INFO] UID карты: ");
      Serial.print(cardUID[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    delay(3000);
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
        return false;
    }

    Serial.println("[INFO] UID успешно записан!");
    return true;
}

void setup() {
    Serial.begin(115200);
    Serial.println("[INFO] Запуск инициализации!");
    SPI.begin();
    mfrc522.PCD_Init();
    stled.begin();

    Serial.println("[INFO] Система готова!");
    stled.set_state(0, 255, 0, 1000); // Зелёное мигание
}

void loop() {
    stled.tick();
    
    if (!cardRead) {
        stled.set_state(128, 0, 128, 500); // Фиолетовое мигание
        if (readCard()) {
            cardRead = true;
            stled.set_state(0, 0, 255); // Синий
        } else {
            stled.set_state(255, 0, 0); // Красный при ошибке
            delay(2000);
        }
    } else {
        stled.set_state(128, 0, 128, 500); // Фиолетовое мигание при записи
Sw  3
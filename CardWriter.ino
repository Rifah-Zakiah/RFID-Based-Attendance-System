#include "arduino_secrets.h"

#include <SPI.h>
#include <MFRC522.h>

/* * WRITER UTILITY
 * 1. Edit NAME_TO_WRITE and ID_TO_WRITE.
 * 2. Upload to ESP32.
 * 3. Open Serial Monitor (115200).
 * 4. Place card to write data.
 */

// --- EDIT THESE FOR EACH STUDENT CARD ---
String NAME_TO_WRITE = "Sadia Iffat"; // Max 16 chars
String ID_TO_WRITE   = "2252421012";  // Student ID (Max 16 chars)

// --- PIN DEFINITIONS (ESP32) ---
#define SS_PIN  5
#define RST_PIN 4   // Changed to 4 (Check your wiring!)

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

void setup() {
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  Serial.println("--- STUDENT CARD WRITER ---");
  Serial.print("Data to write: ");
  Serial.print(ID_TO_WRITE); 
  Serial.print(" | ");
  Serial.println(NAME_TO_WRITE);
  Serial.println("Place card to write...");
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

  Serial.println("Card Detected. Writing...");
  
  if (writeBlock(4, ID_TO_WRITE) && writeBlock(5, NAME_TO_WRITE)) {
    Serial.println("SUCCESS! Data written to card.");
    Serial.println("Please remove card and reset to write next one.");
    delay(3000); 
  } else {
    Serial.println("FAILED to write.");
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

bool writeBlock(byte blockAddr, String data) {
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);

  // Authenticate using Key A
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Auth failed: "); Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }

  // Prepare buffer (pad with spaces)
  for(int i=0; i<16; i++) {
    if(i < data.length()) buffer[i] = data[i];
    else buffer[i] = ' '; // Pad with space
  }

  // Write block
  status = mfrc522.MIFARE_Write(blockAddr, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Write failed: "); Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }
  return true;
}
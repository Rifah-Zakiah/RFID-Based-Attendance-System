#include "arduino_secrets.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// --- CONFIGURATION ---
const char* ssid = "Rifah";
const char* password = "2252421024";
String GOOGLE_SCRIPT_ID = "AKfycbzZbg81VE23J_k7vH8ESI26IzuK2Ni8Ee1C-o5hEZH0FbVD9Q-fxP2WEau2OA9DCzTRhQ"; // From Google Apps Script Deployment

// Admin password (Faculty password to allow card registration)
const String ADMIN_REG_PASS = "9999"; 

// --- PIN DEFINITIONS ---
#define SS_PIN    5
#define RST_PIN   4   // Changed to 4 to avoid conflict with I2C SCL
#define BUZZER_PIN 15

// Keypad Configuration (4x4)
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {32, 33, 25, 26}; 
byte colPins[COLS] = {27, 14, 12, 13}; 

// --- OBJECTS ---
MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2); 
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
MFRC522::MIFARE_Key key;

// --- VARIABLES ---
String readName = "";
String readID = "";

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Init LCD
  lcd.init();
  lcd.backlight();
  lcd.print("System Booting..");

  // Init SPI & RFID
  SPI.begin();
  mfrc522.PCD_Init();
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF; // Default key

  // Init WiFi
  WiFi.begin(ssid, password);
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  lcd.clear();
  lcd.print("WiFi Connected");
  delay(1000);
  lcd.clear();
}

void loop() {
  lcd.setCursor(0, 0);
  lcd.print("Swap Card OR");
  lcd.setCursor(0, 1);
  lcd.print("Press A for Key");

  char key = keypad.getKey();
  
  // --- OPTION 1: KEYPAD ENTRY (No Card) ---
  if (key == 'A') {
    handleManualEntry();
    return;
  }

  // --- OPTION 2: CARD SWIPE ---
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    String uid = getUID();
    Serial.println("Card Detected: " + uid);
    
    lcd.clear();
    lcd.print("Checking...");
    
    // 1. Check Authorization
    String status = sendRequest("action=check&uid=" + uid);
    
    if (status.startsWith("authorized")) {
      // 2. Log Attendance
      lcd.clear();
      lcd.print("Authorized!");
      beep(1);
      
      // Extract Name from response "authorized,John"
      int commaIndex = status.indexOf(',');
      String userName = (commaIndex != -1) ? status.substring(commaIndex + 1) : "Student";
      
      lcd.setCursor(0, 1);
      lcd.print("Hi " + userName);
      
      sendRequest("action=log&uid=" + uid + "&name=" + userName);
      delay(2000);
    } 
    else {
      // 3. Unauthorized - Offer Registration
      lcd.clear();
      lcd.print("Unknown Card");
      beep(3);
      delay(1500);
      handleRegistration(uid);
    }
    
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
}

// --- WORKFLOW FUNCTIONS ---

void handleManualEntry() {
  lcd.clear();
  lcd.print("Enter Passcode:");
  String pass = getPinInput(true); // true = mask input with *
  
  lcd.clear();
  lcd.print("Enter Student ID:"); // CHANGED TO STUDENT ID
  String id = getPinInput(false);
  
  lcd.clear();
  lcd.print("Sending...");
  String response = sendRequest("action=manual&pass=" + pass + "&id=" + id);
  
  lcd.clear();
  if (response == "logged_manual") {
    lcd.print("Attendance OK");
    beep(1);
  } else if (response == "wrong_pass") {
    lcd.print("Wrong Passcode");
    beep(3);
  } else {
    lcd.print("Error");
  }
  delay(2000);
}

void handleRegistration(String uid) {
  lcd.clear();
  lcd.print("Reg? 1:Yes 2:No");
  
  while(true) {
    char k = keypad.getKey();
    if (k == '2') return; // Cancel
    if (k == '1') break;  // Proceed
  }
  
  // Admin Check
  lcd.clear();
  lcd.print("Admin Pass:");
  String adminPass = getPinInput(true);
  
  if (adminPass != ADMIN_REG_PASS) {
    lcd.clear();
    lcd.print("Wrong Admin Pass");
    beep(3);
    delay(2000);
    return;
  }
  
  lcd.clear();
  lcd.print("Reading Card...");
  
  if (readCardData(4, readID) && readCardData(5, readName)) {
    lcd.clear();
    lcd.print("Reg: " + readName);
    lcd.setCursor(0, 1);
    lcd.print("Confirm? #");
    
    while(keypad.getKey() != '#'); // Wait for confirmation
    
    lcd.clear();
    lcd.print("Registering...");
    
    String url = "action=register&uid=" + uid + "&id=" + readID + "&name=" + readName;
    String response = sendRequest(url);
    
    lcd.clear();
    if (response == "registered") {
      lcd.print("Success!");
      beep(2);
    } else if (response == "already_registered") {
      lcd.print("Already Exists");
    } else {
      lcd.print("Failed");
    }
  } else {
    lcd.clear();
    lcd.print("Read Failed");
    lcd.setCursor(0, 1);
    lcd.print("Hold Card Steady");
  }
  delay(2000);
}

// --- HELPER FUNCTIONS ---

String getUID() {
  String content= "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  return content;
}

String getPinInput(bool mask) {
  String input = "";
  lcd.setCursor(0, 1);
  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key == '#') break; // Enter
      if (key == '*') {      // Clear
        input = "";
        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);
      } else {
        input += key;
        if (mask) lcd.print("*");
        else lcd.print(key);
      }
    }
  }
  return input;
}

String sendRequest(String params) {
  if(WiFi.status() == WL_CONNECTED){
    HTTPClient http;
    String url = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec?" + params;
    
    http.begin(url.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    int httpCode = http.GET();
    String payload = "";
    
    if (httpCode > 0) {
      payload = http.getString();
    } else {
      payload = "error_http";
    }
    http.end();
    return payload;
  }
  return "error_wifi";
}

bool readCardData(byte blockAddr, String &output) {
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) return false;

  status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) return false;

  output = "";
  for (byte i = 0; i < 16; i++) {
    if(buffer[i] != 0 && buffer[i] != ' ') output += (char)buffer[i];
  }
  output.trim();
  return true;
}

void beep(int times) {
  for(int i=0; i<times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}
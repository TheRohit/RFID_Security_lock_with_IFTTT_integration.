#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include<Servo.h>

Servo myServo;

#define SS_PIN D8
#define RST_PIN D4
#define BTN_PIN D1
#define SLN_PIN D2

MFRC522 mfrc522(SS_PIN, RST_PIN);
unsigned long uidDec, uidDecTemp;
int ARRAYindexUIDcard;
int EEPROMstartAddr;
long adminID = 1122539531;
bool beginCard = 0;
bool addCard = 1;
bool skipCard = 0;
int LockSwitch;
unsigned long CardUIDeEPROMread[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
int PiezoPin = D3;

char auth[] = "Mj13WU-nhVmAZC1hXbAuXFP0v5X1wUpm";
char ssid[] = "SSID";
char pass[] = "password";

WidgetLCD lcd(V0);

void setup() {
  Serial.begin(115200);
  pinMode(SLN_PIN, OUTPUT); digitalWrite(SLN_PIN, LOW);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(PiezoPin, OUTPUT);

  SPI.begin();
  mfrc522.PCD_Init();

  Blynk.begin(auth, ssid, pass);
  lcd.clear(); //Use it to clear the LCD Widget
  EEPROM.begin(512);
  DisplayWAiT_CARD();
  EEPROMreadUIDcard();

  digitalWrite(PiezoPin, HIGH), delay(100), digitalWrite(PiezoPin, LOW);

}

void loop() {

  digitalWrite(SLN_PIN, LOW);
  myServo.attach(D2);     // Set servo atach which is connected to digital pin 3 
  myServo.write(90); // Gate Close Move the servo to the home position

  if (digitalRead(BTN_PIN) == LOW) {
    digitalWrite(SLN_PIN, HIGH); //unlock
    lcd.print(0, 0, " BUTTON UNLOCK ");
    lcd.print(0, 1, "   DOOR OPEN   ");
    digitalWrite(PiezoPin, HIGH), delay(200), digitalWrite(PiezoPin, LOW);
    delay(2000);
    DisplayWAiT_CARD();
  }

  if (beginCard == 0) {
    if ( ! mfrc522.PICC_IsNewCardPresent()) {  //Look for new cards.
      Blynk.run();
      return;
    }

    if ( ! mfrc522.PICC_ReadCardSerial()) {  //Select one of the cards.
      Blynk.run();
      return;
    }
  }

  //Read "UID".
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uidDecTemp = mfrc522.uid.uidByte[i];
    uidDec = uidDec * 256 + uidDecTemp;
  }

  if (beginCard == 1 || LockSwitch > 0)EEPROMwriteUIDcard();  //uidDec == adminID

  if (LockSwitch == 0) {
    //CardUIDeEPROMread.
    for (ARRAYindexUIDcard = 0; ARRAYindexUIDcard <= 9; ARRAYindexUIDcard++) {
      if (CardUIDeEPROMread[ARRAYindexUIDcard] > 0) {
        if (CardUIDeEPROMread[ARRAYindexUIDcard] == uidDec) {
           myServo.write (- 90);
          lcd.print(0, 0, "CARD ACCESS OPEN");
          lcd.print(3, 1, uidDec);
          
          digitalWrite(PiezoPin, HIGH), delay(200), digitalWrite(PiezoPin, LOW);
          delay(2500);
          break;
        }
      }
    }

    if (ARRAYindexUIDcard == 10) {
      lcd.print(0, 0, " Card not Found ");
      lcd.print(0, 1, "                ");
      lcd.print(0, 1, "ID : ");
      lcd.print(5, 1, uidDec);
      for (int i = 0; i <= 2; i++)delay(100), digitalWrite(PiezoPin, HIGH), delay(100), digitalWrite(PiezoPin, LOW);
      digitalWrite(SLN_PIN, LOW);  //lock();
      delay(2500);
    }

    ARRAYindexUIDcard = 0;
    DisplayWAiT_CARD();
  }

  Blynk.run();
}

BLYNK_WRITE(V1) {
  int a = param.asInt();
  if (a == 1) beginCard = 1; else beginCard = 0;
}

BLYNK_WRITE(V2) {
  int a = param.asInt();
  if (a == 1) {
    skipCard = 1;
    if (EEPROMstartAddr / 5 < 10) EEPROMwriteUIDcard();
  } else {
    skipCard = 0;
  }
}

BLYNK_WRITE(V3) {
  int a = param.asInt();
  if (a == 1) {
     myServo.write (-90); //unlock
    lcd.print(0, 0, " APP UNLOCK OK ");
    lcd.print(0, 1, "   DOOR OPEN   ");
    digitalWrite(PiezoPin, HIGH), delay(200), digitalWrite(PiezoPin, LOW);
    delay(2500);
    DisplayWAiT_CARD();
  } 
}

void EEPROMwriteUIDcard() {

  if (LockSwitch == 0) {
    lcd.print(0, 0, " START REC CARD ");
    lcd.print(0, 1, "PLEASE TAG CARDS");
    delay(2000);
  }

  if (LockSwitch > 0) {
    if (skipCard == 1) {  //uidDec == adminID
      lcd.print(0, 0, "   SKIP RECORD   ");
      lcd.print(0, 1, "                ");
      lcd.print(0, 1, "   label : ");
      lcd.print(11, 1, EEPROMstartAddr / 5);
      EEPROMstartAddr += 5;
      skipCard = 0;
    } else {
      Serial.println("writeCard");
      EEPROM.write(EEPROMstartAddr, uidDec & 0xFF);
      EEPROM.write(EEPROMstartAddr + 1, (uidDec & 0xFF00) >> 8);
      EEPROM.write(EEPROMstartAddr + 2, (uidDec & 0xFF0000) >> 16);
      EEPROM.write(EEPROMstartAddr + 3, (uidDec & 0xFF000000) >> 24);
      EEPROM.commit();
      delay(10);
      lcd.print(0, 1, "                ");
      lcd.print(0, 0, "RECORD OK! IN   ");
      lcd.print(0, 1, "MEMORY : ");
      lcd.print(9, 1, EEPROMstartAddr / 5);
      EEPROMstartAddr += 5;
      delay(2000);
    }
  }

  LockSwitch++;

  if (EEPROMstartAddr / 5 == 10) {
    lcd.clear();
    lcd.print(0, 0, "RECORD FINISH");
    delay(2000);
    EEPROMstartAddr = 0;
    uidDec = 0;
    ARRAYindexUIDcard = 0;
    EEPROMreadUIDcard();
  }
}

void EEPROMreadUIDcard() {
  for (int i = 0; i <= 9; i++) {
    byte val = EEPROM.read(EEPROMstartAddr + 3);
    CardUIDeEPROMread[ARRAYindexUIDcard] = (CardUIDeEPROMread[ARRAYindexUIDcard] << 8) | val;
    val = EEPROM.read(EEPROMstartAddr + 2);
    CardUIDeEPROMread[ARRAYindexUIDcard] = (CardUIDeEPROMread[ARRAYindexUIDcard] << 8) | val;
    val = EEPROM.read(EEPROMstartAddr + 1);
    CardUIDeEPROMread[ARRAYindexUIDcard] = (CardUIDeEPROMread[ARRAYindexUIDcard] << 8) | val;
    val = EEPROM.read(EEPROMstartAddr);
    CardUIDeEPROMread[ARRAYindexUIDcard] = (CardUIDeEPROMread[ARRAYindexUIDcard] << 8) | val;

    ARRAYindexUIDcard++;
    EEPROMstartAddr += 5;
  }

  ARRAYindexUIDcard = 0;
  EEPROMstartAddr = 0;
  uidDec = 0;
  LockSwitch = 0;
  DisplayWAiT_CARD();
}

void DisplayWAiT_CARD() {
  lcd.print(0, 0, "   ATTACH THE   ");
  lcd.print(0, 1, "      CARD      ");
}

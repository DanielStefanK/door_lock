#include <SPI.h>
#include <Wire.h> 
#include <MFRC522.h>
#include <EEPROM.h> 
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

#define RST_PIN   9
#define SS_PIN    10
#define BACKLIGHT_PIN 2

int beep = 7;               //active buzzer
int door = 8;               // door buzzer

uint8_t successRead;        //if read or not 
byte readCard[4];           //stores the read card
byte masterCard[4];         //stores the master card read from memory
byte storedCard[4];         // sotres the memory read card
bool match = false;

int data = 2;               //data pin of the matrix
int load = 3;               //load pin of the matrix
int clock = 4;              //clockpin of the matrix
int maxInUse = 1;           // if the matrix is in use

int state = 0;              //state of the application

unsigned long previousMillis = 0; // intervasl for loading animation

MFRC522 mfrc522(SS_PIN, RST_PIN); //setup of the RFID reader 
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); 

void setup() {
  SPI.begin(); 
  Serial.begin(9600);
  while (!Serial) {
    ;
  }
  
  lcd.backlight();              
  lcd.begin(16,2);                   
  lcd.clear();
  lcd.home (); 
  
  pinMode(beep, OUTPUT);
  pinMode(door, OUTPUT);
  
  mfrc522.PCD_Init();

  //for (int i = 0; i < 512; i++)
  //  EEPROM.write(i, 0);
  // check for mastercard in storage of nothing found 
  if (EEPROM.read(1) != 143) {
    lcd.clear();
    lcd.home (); 
    lcd.print("No Mastercard.");
    lcd.setCursor ( 0, 1 );
    lcd.print("Please scan one."); 
    do {
      successRead = getID();
    }
    while (!successRead);
    for ( uint8_t j = 0; j < 4; j++ ) {
      EEPROM.write( 2 + j, readCard[j] );
    }
    EEPROM.write(1, 143);
    lcd.clear();
    lcd.home ();
    lcd.print("Mastercard");
    lcd.setCursor ( 0, 1 );
    lcd.print("scanned");
    tone(beep,2000,100);
    delay(110);
    tone(beep,4000,300);
  }
  
  for ( uint8_t i = 0; i < 4; i++ ) {      
    masterCard[i] = EEPROM.read(2 + i);  
  }
  previousMillis = millis();
  lcd.clear();
}


void loop() {
  switch(state) {
    case 0: {
      lcd.clear();
      lcd.home ();
      lcd.print("Scan Tag");
      do {
        successRead = getID();
        noBacklight();
      } while (!successRead);
      
      
      if(isMaster(readCard)){ 
        previousMillis = millis();
        lcd.backlight();
        state = 1;
      }else {
        if ( findID(readCard) ) { // If not, see if the card is in the EEPROM
          previousMillis = millis();
          lcd.backlight();
          digitalWrite(door, HIGH);
          lcd.clear();
          lcd.home ();
          lcd.print("Tag accepted");
          lcd.setCursor ( 0, 1 );
          lcd.print("Please enter");
          tone(beep, 4000,100);
          delay(110);
          tone(beep, 5900,300);
          delay(2000);
          digitalWrite(door, LOW);
        }
        else {      // If not, show that the ID was not valid
          lcd.clear();
          lcd.home ();
          lcd.print("Tag invalid");
          lcd.setCursor ( 0, 1 );
          lcd.print("");
          tone(beep, 3500,400);
          delay(1600);
        }
      }
      }
      break;
    case 1: {
        lcd.clear();
        lcd.home ();
        lcd.print("Programmingmode");
        lcd.setCursor ( 0, 1 );
        lcd.print("remove/add tags");
        tone(beep, 6000,100);
       do {
        successRead = getID();
        
      } while (!successRead);
      if(isMaster(readCard)){
        state = 0;
        tone(beep, 4000,100);
      } else {
        if ( findID(readCard) ) {
          deleteID(readCard);
          lcd.setCursor ( 0, 1 );
          lcd.print("Tag removed    ");
          tone(beep, 5900,100);
          delay(110);
          tone(beep, 3500,300);
          delay(2000);
        }
        else {
          writeID(readCard);
          lcd.setCursor ( 0, 1 );
          lcd.print("Tag added      ");
          tone(beep, 4000,100);
          delay(110);
          tone(beep, 5900,300);
          delay(2000);
        }
      }
      previousMillis = millis();
      }
      break;
    }
}

uint8_t findIDSLOT( byte find[] ) {
  uint8_t count = EEPROM.read(0);
  for ( uint8_t i = 1; i <= count; i++ ) {
    readID(i);
    if ( checkTwo( find, storedCard ) ) { 
      return i;
      break;
    }
  }
}

void failedWrite() {
  incorrect();
}

void successDelete() {
  
}


void successWrite() {
  
}

void readID( uint8_t number ) {
  uint8_t start = (number * 4 ) + 2;    
  for ( uint8_t i = 0; i < 4; i++ ) {    
    storedCard[i] = EEPROM.read(start + i);   
  }
}

boolean findID( byte find[] ) {
  uint8_t count = EEPROM.read(0);     // Read the first Byte of EEPROM that

  for ( uint8_t i = 1; i <= count; i++ ) {
    readID(i);          // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      return true;
      break;  // Stop looking we found it
    }
    else {    // If not, return false
    }
  }
  return false;
}

void writeID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we write to the EEPROM, check to see if we have seen this card before!
    uint8_t num = EEPROM.read(0);     // Get the numer of used spaces, position 0 stores the number of ID cards
    uint8_t start = ( num * 4 ) + 6;  // Figure out where the next slot starts
    num++;                // Increment the counter by one
    EEPROM.write( 0, num );     // Write the new count to the counter
    for ( uint8_t j = 0; j < 4; j++ ) {   // Loop 4 times
      EEPROM.write( start + j, a[j] );  // Write the array values to EEPROM in the right position
    }
    successWrite();
    Serial.println(F("Succesfully added ID record to EEPROM"));
  }
  else {
    failedWrite();
    Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
  }
}

void deleteID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we delete from the EEPROM, check to see if we have this card!
    failedWrite();      // If not
    Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
  }
  else {
    uint8_t num = EEPROM.read(0);   // Get the numer of used spaces, position 0 stores the number of ID cards
    uint8_t slot;       // Figure out the slot number of the card
    uint8_t start;      // = ( num * 4 ) + 6; // Figure out where the next slot starts
    uint8_t looping;    // The number of times the loop repeats
    uint8_t j;
    uint8_t count = EEPROM.read(0); // Read the first Byte of EEPROM that stores number of cards
    slot = findIDSLOT( a );   // Figure out the slot number of the card to delete
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;      // Decrement the counter by one
    EEPROM.write( 0, num );   // Write the new count to the counter
    for ( j = 0; j < looping; j++ ) {         // Loop the card shift times
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));   // Shift the array values to 4 places earlier in the EEPROM
    }
    for ( uint8_t k = 0; k < 4; k++ ) {         // Shifting loop
      EEPROM.write( start + j + k, 0);
    }
    successDelete();
    Serial.println(F("Succesfully removed ID record from EEPROM"));
  }
}

boolean isMaster( byte test[] ) {
  if ( checkTwo( test, masterCard ) )
    return true;
  else
    return false;
}

boolean checkTwo ( byte a[], byte b[] ) {
  if ( a[0] != 0 )      // Make sure there is something in the array first
    match = true;       // Assume they match at first
  for ( uint8_t k = 0; k < 4; k++ ) {   // Loop 4 times
    if ( a[k] != b[k] )     // IF a != b then set match = false, one fails, all fail
      match = false;
  }
  if ( match ) {      // Check to see if if match is still true
    return true;      // Return true
  }
  else  {
    return false;       // Return false
  }
}

void checkAni(){
  tone(beep,2000,100);
  delay(110);
  tone(beep,4000,300);
  
}
void incorrect(){
  tone(beep, 2000,100);
  delay(110);
  tone(beep,1000,300);

}
void loadingAni(){
  
}

uint8_t getID() {
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  Serial.println(F("Scanned PICC's UID:"));
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
} 
void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);
  if (v == 0x91)
    Serial.print(F(" = v1.0"));
  else if (v == 0x92)
    Serial.print(F(" = v2.0"));
  else
    Serial.print(F(" (unknown)"));
  Serial.println("");
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
  }
}

void noBacklight(){
  if(millis() - previousMillis > 10000){
    lcd.noBacklight();
  }
}


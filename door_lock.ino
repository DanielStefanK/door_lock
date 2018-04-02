#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h> 
#include <MaxMatrix.h>

#define RST_PIN   9
#define SS_PIN    10

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
int loading = 0;                  // row of the bar whil loading
bool up = true;                   //either going up or down

MFRC522 mfrc522(SS_PIN, RST_PIN); //setup of the RFID reader 
MaxMatrix m(data, load, clock, maxInUse);// setup of the matrix

void setup() {
  SPI.begin(); 
  Serial.begin(9600);
  while (!Serial) {
    ;
  }
  
  m.init(); 
  m.setIntensity(1);
  m.clear();
  
  pinMode(beep, OUTPUT);
  pinMode(door, OUTPUT);
  
  mfrc522.PCD_Init();

  //for (int i = 0; i < 512; i++)
  //EEPROM.write(i, 0);
  // check for mastercard in storage of nothing found 
  if (EEPROM.read(1) != 143) {
    do {
      successRead = getID();
      loadingAni();
    }
    while (!successRead);
    for ( uint8_t j = 0; j < 4; j++ ) {
      EEPROM.write( 2 + j, readCard[j] );
    }
    EEPROM.write(1, 143);
  }
  for ( uint8_t i = 0; i < 4; i++ ) {      
    masterCard[i] = EEPROM.read(2 + i);  
  }
  
}


void loop() {
  switch(state) {
    case 0: {
      do {
        successRead = getID();
      } while (!successRead);
      if(isMaster(readCard)){
        state = 1;
        tone(beep, 5000,100);
      }else {
        if ( findID(readCard) ) { // If not, see if the card is in the EEPROM
          digitalWrite(door, HIGH);
          checkAni();
          delay(2000);
          digitalWrite(door, LOW);
        }
        else {      // If not, show that the ID was not valid
          incorrect();
        }
      }
      }
      break;
    case 1: {
       do {
        successRead = getID();
        loadingAni();
        
      } while (!successRead);
      if(isMaster(readCard)){
        state = 0;
        tone(beep, 5000,100);
      } else {
        if ( findID(readCard) ) {
          incorrect();
          deleteID(readCard);
        }
        else {
          checkAni();
          writeID(readCard);
        }
      }
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
  tone(beep,3000,100);
  m.setDot(0,4,1);
  delay(100);
  tone(beep,5000,300);
  m.setDot(1,5,1);
  delay(90);
  m.setDot(2,6,1);
  delay(80);
  m.setDot(3,5,1);
  delay(60);
  m.setDot(4,4,1);
  delay(40);
  m.setDot(5,3,1);
  delay(20);
  m.setDot(6,2,1);
  delay(10);
  m.setDot(7,1,1);
  delay(1000);
  m.clear();
}
void incorrect(){
  tone(beep, 5000,100);
  
  m.setDot(1,1,1);
  delay(100);
  m.setDot(2,2,1);
  tone(beep,3000,300);
  delay(80);
  m.setDot(3,3,1);
  delay(60);
  m.setDot(4,4,1);
  delay(40);
  m.setDot(5,5,1);
  delay(20);
  m.setDot(6,6,1);
  delay(100);
  m.setDot(6,1,1);
  delay(90);
  m.setDot(5,2,1);
  delay(80);
  m.setDot(4,3,1);
  delay(60);
  m.setDot(3,4,1);
  delay(40);
  m.setDot(2,5,1);
  delay(20);
  m.setDot(1,6,1);
  delay(1000);
  m.clear();
}
void loadingAni(){
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 50) {
    previousMillis = currentMillis;
    m.setColumnAll(loading, 255);
    delay(50);
    m.setColumn(loading, 0);
    if(up && loading==7){
      up = false;
      loading--;
    } else if (up && loading<7){
      loading++;
    } else if (!up && loading==0){
      up = true;
      loading++;
    } else {
      loading--;
    }
    m.clear();
  }
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


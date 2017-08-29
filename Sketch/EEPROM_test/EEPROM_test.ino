#include <EEPROM.h>

#define commandControlLength 3

#define connectionCategory 1
#define connectionValidatePassCommand 1
#define connectionSetPassCommand 2

const char eepromHeader[] = {
  1,
  6,
  3,
  8,
  7,
  9
};
char passphrase[] = { //121212
  0x31,
  0x32,
  0x31,
  0x32,
  0x31,
  0x32
};

#define securityCategory 2
#define securityLockCommand 1
#define securityUnlockCommand 2
#define alarmCommand 3

/*
 * Write passphrase to EEPROM. Writes one byte after the EEPROM Header. Separated from EEPROM Header by one byte of value 0x0. Ends with a byte of value 0x0.
 */
void writePassToEEPROM() {
  for (uint8_t i = 0; i < sizeof(passphrase); i++) {
    EEPROM.write(sizeof(eepromHeader)+1+i, passphrase[i]);
  }
  EEPROM.write(sizeof(eepromHeader)+sizeof(passphrase)+1, 0x00);
}

/*
 * Write EEPROM Header at beginning of EEPROM. Write 0x00 byte after header.
 */
void writeHeaderToEEPROM() {
  for (uint8_t i = 0; i < sizeof(eepromHeader); i++)
    EEPROM.write(i, eepromHeader[i]);
  EEPROM.write(sizeof(eepromHeader), 0x00);
}

/*
 * Read passphrase from EEPROM
 */
void readPassFromEEPROM() {
  for (uint8_t i = 0; i < sizeof(passphrase); i++) {
    passphrase[i] = EEPROM.read(sizeof(eepromHeader)+1+i);
    
    /*
    Serial.print(i);
    Serial.print("r ");
    Serial.println(passphrase[i]);
    */

  }
}

/*
 * Reset all EEPROM to 0xff (255). 0xff (255) is the default eeprom value.
 */
void resetEEPROM() {
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0xff); 
  }
}

/*
 * Check for valid EEPROM Header at beginning of EEPROM.
 */
bool checkValidEEPROMHeader() {
  for (uint8_t i = 0; i < sizeof(eepromHeader); i++) {
    if (EEPROM.read(i) != eepromHeader[i])
      return false;
  }
  return true;
}

bool checkValidPass(uint8_t* pass) {
  readPassFromEEPROM();

  if (memcmp(passphrase, pass, sizeof(passphrase)) == 0)
    return true;
  else
    return false;
}

/*
 * Check BLE Characteristic for HM-10 BLE Module and process any commands. 
 */
void checkBLECharacteristic() {
  char command[100];
  memset(command, 0, sizeof(command));

  /*
  //Validate pass of 121212
  char test[] {
    0x31,
    0x31,
    0x31,
    0x32,
    0x31,
    0x32,
    0x31,
    0x32
  };
  */

  /*
  //Change pass from 121212 to 221212
  char test[] {
    0x31,
    0x32,
    0x31,
    0x32,
    0x31,
    0x32,
    0x31,
    0x32,
    0x32, //new pass
    0x32,
    0x31,
    0x32,
    0x31,
    0x32
  };
  */

  /*
  //Lock car once 210XXXXXX
  char test[] {
    0x32,
    0x31,
    0x30,
    0x31,
    0x32,
    0x31,
    0x32,
    0x31,
    0x32
  };
  */
  
  //Lock car twice 211XXXXXX
  char test[] {
    0x32,
    0x31,
    0x31,
    0x31,
    0x32,
    0x31,
    0x32,
    0x31,
    0x32
  };
  

  /*
  //Read in actual command from serial
  uint8_t cLength = 0;
  while (bleSerial.available()) {
    command[cLength] = bleSerial.read();
    cLength++;
  }
  */

  //DEBUG copy over test command from test variable
  Serial.print("Received command (ASCII): ");
  uint8_t cLength = 0;
  for (cLength = 0; cLength < sizeof(test); cLength++) {
    command[cLength] = test[cLength];

    Serial.print(command[cLength]);
  }
  Serial.println();



  if (cLength) {
    uint8_t commandControl[commandControlLength];
    for (uint8_t i = 0; i < commandControlLength; i++) {
      char buffer[1];
      buffer[0] = command[i];
      commandControl[i] = atoi(buffer);

      Serial.print(i);
      Serial.print(" ");
      Serial.println(commandControl[i]);
    }
    
    switch (commandControl[0]) {
      case connectionCategory:
        switch (commandControl[1]) {
          case connectionValidatePassCommand: //110XXXXXX

            if (checkValidPass((uint8_t*)&command[commandControlLength])) {
              Serial.println(0);
            }
            else {
              Serial.println(1);
            }

            break;
          case connectionSetPassCommand: //120XXXXXXYYYYYY
          
            if (checkValidPass((uint8_t*)&command[commandControlLength])) {
              Serial.print("Setting new passphrase: ");
              for (uint8_t i = 0; i < sizeof(passphrase); i++) {
                passphrase[i] = command[commandControlLength+sizeof(passphrase)+i];

                Serial.print(passphrase[i]);
              }
              writePassToEEPROM();

              Serial.println();
              Serial.println(0);
            } else {
              Serial.println(1);
            }


            break;
        }
      break;
      case securityCategory:
        switch (commandControl[1]) {
          case securityLockCommand: //210XXXXXX
            if (checkValidPass((uint8_t*)&command[commandControlLength])) {
              Serial.println("lock once");
              Serial.println(0);
              if (commandControl[2]) { //received "211XXXXXX"
                Serial.println("lock twice");
              }
            }
            else {
              Serial.println(1);
            }
            break;
        }
        break;
    }
  }
}

/*
 * Setup remote lock/unlock by BLE.
 */
void remote_access_setup() {

  Serial.begin(9600);

  //Write default passphrase to EEPROM if invalid header
  if (!checkValidEEPROMHeader()) {
    Serial.println("Invalid eeprom header. Writing header and default passphrase.");
    writePassToEEPROM();
    writeHeaderToEEPROM();
  } else {
    readPassFromEEPROM();
  }

  Serial.print("Current passphrase: ");
  for (uint8_t i = 0; i < sizeof(passphrase); i++) {
    Serial.print(passphrase[i]);
  }

  Serial.println();
  //ble_setup();

  checkBLECharacteristic();
}

void setup() {
  remote_access_setup();
}

void loop()
{
}

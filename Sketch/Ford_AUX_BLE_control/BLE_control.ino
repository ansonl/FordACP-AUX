/* 
 * BLE Control for Ford ACP AUX interface by Anson Liu
 *
 * Control vehicle locks through BLE.
 * Use 74HC595 latching shift register to control key fob.
 * Use HM-10 bluetooth module to communicate over BLE.
 * For best results, set AT+TYPE3 to enable auth+pair+bond.
 *
 */

#include <EEPROM.h>
#include <AltSoftSerial.h>
#include <Shifty.h>
AltSoftSerial bleSerial;
#define cdEnablePin 3

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
  6
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
#define securityAlarmCommand 3

#define lockoutInterval 10000
bool lockout;
unsigned long lockoutMillis;

char atSleep[] = "AT+SLEEP\r\n";

//Shift register setup
Shifty shift;
const uint8_t shiftRegisterEnablePin = 17;
const uint8_t lockPin = 0;
const uint8_t unlockPin = 1;
const uint8_t alarmPin = 2;

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
  //Determine if lockout mode for connectionCategory. Return error code 2 if lockout
  if (lockout && (unsigned long)(millis() - lockoutMillis) >= lockoutInterval)
    lockout = false;

  char command[50];
  memset(command, 0, sizeof(command));

  uint8_t cLength = 0;
  while (bleSerial.available()) {
    command[cLength] = bleSerial.read();
    cLength++;
    delay(100); //Wait for a short time ~100ms with HM-10 before checking to see if serial has data available. Checking too fast may result in loss of data (data not available yet).
  }

  if (cLength) {
    uint8_t commandControl[commandControlLength];
    for (uint8_t i = 0; i < commandControlLength; i++) {
      char buffer[1];
      buffer[0] = command[i];
      commandControl[i] = atoi(buffer);
    }

    //response buffer
    char response[5];
    memset(response, 0, sizeof(response));
    for (uint8_t i = 0; i < 3; i++) {
      response[i] = command[i];
    }

    //return if lockout mode active
    if (lockout) {
      response[3] = 0x32;
      bleSerial.print(response);
      return;
    }

    switch (commandControl[0]) {
      case connectionCategory:
        switch (commandControl[1]) {
          case connectionValidatePassCommand: //110XXXXXX (ascii)

            if (checkValidPass((uint8_t*)&command[commandControlLength])) {              
              response[3] = 0x30;
              bleSerial.print(response);
            }
            else {
              response[3] = 0x31;
              bleSerial.print(response);
              lockoutMillis = millis();
              lockout = true;
            }

            break;
          case connectionSetPassCommand: //120XXXXXXYYYYYY
          
            if (checkValidPass((uint8_t*)&command[commandControlLength])) {
              for (uint8_t i = 0; i < sizeof(passphrase); i++) {
                passphrase[i] = command[commandControlLength+sizeof(passphrase)+i];
              }
              writePassToEEPROM();
              response[3] = 0x30;
              bleSerial.print(response);
            } else {
              response[3] = 0x31;
              bleSerial.print(response);
              lockoutMillis = millis();
              lockout = true;
            }


            break;
        }
      break;
      case securityCategory:
        switch (commandControl[1]) {
          case securityLockCommand: //210XXXXXX
            if (checkValidPass((uint8_t*)&command[commandControlLength])) {
              
              digitalWrite(shiftRegisterEnablePin, HIGH);
              shift.writeBit(lockPin, HIGH);
              delay(500);
              shift.writeBit(lockPin, LOW);
              delay(1000);

              if (commandControl[2]) { //221XXXXXX
                shift.writeBit(lockPin, HIGH);
                delay(500);
                shift.writeBit(lockPin, LOW);
                delay(1000);
              }
              response[3] = 0x30;
              bleSerial.print(response);
            }
            else {
              response[3] = 0x31;
              bleSerial.print(response);
            }
            break;
          case securityUnlockCommand: //220XXXXXX
            if (checkValidPass((uint8_t*)&command[commandControlLength])) {

              digitalWrite(shiftRegisterEnablePin, HIGH);
              shift.writeBit(unlockPin, HIGH);
              delay(500);
              shift.writeBit(unlockPin, LOW);
              delay(1000);

              if (commandControl[2]) { //221XXXXXX
                shift.writeBit(unlockPin, HIGH);
                delay(500);
                shift.writeBit(unlockPin, LOW);
                delay(1000);
              }
              response[3] = 0x30;
              bleSerial.print(response);
            }
            else {
              response[3] = 0x31;
              bleSerial.print(response);
            }
            break;
          case securityAlarmCommand: //230XXXXXX
            if (checkValidPass((uint8_t*)&command[commandControlLength])) {

              digitalWrite(shiftRegisterEnablePin, HIGH);
              shift.writeBit(alarmPin, HIGH);
              delay(500);
              shift.writeBit(alarmPin, LOW);
              delay(1000);

              response[3] = 0x30;
              bleSerial.print(response);
            }
            else {
              response[3] = 0x31;
              bleSerial.print(response);
            }
            break;
        }
        break;
    default:
      response[3] = 0x32;
      bleSerial.print(response);
      break;

      digitalWrite(shiftRegisterEnablePin, LOW);
    }
  }
}

/*
 * Put BLE to sleep mode. MLT-BT-5 requires \r and then \n after AT commands. 
 */
void send_AT_sleep() {
  //TODOS require PCB change
  //TODO: Use STATE pin to determine if BLE module is connected to devices to see if it can take AT commands.
  //TODO: Use EN pin to break connection to force module to take AT commands. 
  bleSerial.print(atSleep); 
}

void headunitOnInterrupt() {
  send_AT_sleep();

  //start and stop timer
  switch(digitalRead(cdEnablePin)) {
    case true:
      MsTimer2::start();
      break;
    case false:
      MsTimer2::stop();
  }
}

/*
 * Setup HM-10 BLE Serial
 */
void ble_setup() {
  //Setup HM-10 BLE serial and put in sleep mode
  bleSerial.begin(9600);
  send_AT_sleep();

  //Setup shift register
  digitalWrite(shiftRegisterEnablePin, LOW);
  pinMode(shiftRegisterEnablePin, OUTPUT);
  shift.setBitCount(8);
  shift.setPins(16, 14, 15); //data,clock,latch
  shift.batchWriteBegin();
  //Set all shift register bits to 0
  for (uint8_t i = 0; i < 8; i++)
    shift.writeBit(i, LOW);
  shift.batchWriteEnd();

  //Set interrupt on CD Enable pin to trigger BLE Sleep
  pinMode(cdEnablePin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(cdEnablePin), headunitOnInterrupt, CHANGE);
}

/*
 * Setup remote lock/unlock by BLE.
 */
void remote_access_setup() {
  //Write default passphrase to EEPROM if invalid header
  if (!checkValidEEPROMHeader()) {
    writePassToEEPROM();
    writeHeaderToEEPROM();
  } else {
    readPassFromEEPROM();
  }

  ble_setup();
}
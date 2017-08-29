/* 
 * Ford Head Unit to iOS AUX Interface v1.2
 *
 * The code in this file is intended to be uploaded to an Arduino Mega2560
 * used in combination with a specialized shield that contains the hardware
 * needed to interface with a Ford head unit and iOS TRRS audio port.
 *
 * Compatible Ford Head Units: Any unit that uses ACP (Automotive Communication
 *                             Protocol) and has a connector for a CD changer.
 *
 *                             Info on ACP:
 *                             http://www.mictronics.de/projects/cdc-protocols/#FordACP
 *                             
 *                             Some examples are:
 *                             4050RDS, 4500, 4600RDS, 5000RDS, 5000RDS EON, 
 *                             6000CD RDS, 6000 MP3, 7000RDS
 *
 *                             These are all head units from the '96 - '05 era.
 *
 *                             The unit used during testing is a 1W1F-18C870-DA 02 03
 *                             from a '99 Ford Expedition.
 *
 * The code for the ACP protocol was originally written by Andrew Hammond for his Yampp
 * and was ported over to the Arduino platform by Krzysztof Pintscher. Please see the 
 * ACP and CD headers for more info.
 *
 * Version 1.2 has been tested to be working with 2007 Ford Escape Hybrid audio system.
 *
 **************************************************************************************
 * Revisions:
 *
 * 2014-11-30 version 1.0 - public release - Dale Thomas
 * 2017-05-07 version 1.1 - Removed Bluetooth functionality and edited ports for use with Arduino UNO. - AL
 * 2017-06-16 version 1.2 - Added iOS AUX inline control commands and timing to loop(). - Anson Liu (ansonliu.com)
 *
 **************************************************************************************
 * Requirements:
 *
 * -TimerOne library
 *
 */


//BLE Control
#include <Shifty.h>
#include <AltSoftSerial.h>
#include <EEPROM.h>

#define connectionCategory 1
#define connectionValidatePassCommand 1
#define connectionSetPassCommand 2
const uint8_t eepromHeader[] = {
  1,
  6,
  3,
  8,
  7,
  6
};
uint8_t passphrase[] = {
  1,
  1,
  1,
  2
};

#define securityCategory 2
#define securityLockCommand 1
#define securityUnlockCommand 2
#define alarmCommand 3
AltSoftSerial bleSerial(2,3);
Shifty shift;
const uint8_t shiftRegisterEnablePin = 17;
const uint8_t lockPin = 0;
const uint8_t unlockPin = 1;
const uint8_t alarmPin = 2;

//Ford ACP
#include <util/delay.h>
#include <TimerOne.h>
typedef unsigned char u08;
typedef unsigned short u16;
typedef unsigned long  u32;

enum InlineControlCommand {
  noCommand,
  cancelCommand,
  playPause,
  nextTrack,
  prevTrack,
  fastForwardTrack,
  rewindTrack,
  activateSiri
};

#ifndef cbi // "clear bit"
 #define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi // "set bit"
 #define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif
#define outp(VAL,ADRESS) ((ADRESS) = (VAL))
#define inp(VAL) (VAL)

#define ACP_UART_BAUD_RATE 	9600
#define ACP_UART_BAUD_SELECT ((u32)((u32)16000000/(ACP_UART_BAUD_RATE*16L)-1))
#define ACP_LISTEN 0
#define ACP_SENDACK 1
#define ACP_MSGREADY 2
#define ACP_WAITACK 3
#define ACP_SENDING 4

const uint8_t switchPin = 7; // Pin 7 connected to pins 2 & 3 (RE/DE) on MAX485
const uint8_t inlineControlPin = 6; // Pin 6 connected to transistor for inline control

uint8_t acp_rx[12];
uint8_t acp_tx[12];
uint8_t acp_rxindex;
uint8_t acp_txindex;
u08 acp_status;
u08 acp_txsize;
u08 acp_timeout;
u08 acp_checksum;
u08 acp_retries;
u08 acp_mode;
uint16_t acp_ltimeout;
boolean rewindState = false;
boolean ffState = false;
boolean wPlayTime = false;
uint8_t currentTrack = 1;
boolean reset_timer = false;

InlineControlCommand lastCommand;

/*
 * Write passphrase to EEPROM. Writes one byte after the EEPROM Header. Separated from EEPROM Header by one byte of value 0x0. Ends with a byte of value 0x0.
 */
void writePassToEEPROM() {
  for (uint8_t i = sizeof(eepromHeader)+1; i < sizeof(eepromHeader)+sizeof(passphrase)+1; i++)
    EEPROM.write(i, passphrase[i-sizeof(eepromHeader)]);
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
bool readPassFromEEPROM() {
  uint8_t passLength = 0;
  while (EEPROM.read(sizeof(eepromHeader)+1+passLength) != 0x00) {
    passLength++;
  }

  passphrase[passLength];
  for (uint8_t i = 0; i < passLength; i++) {
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

bool checkValidPass(char* pass) {
  readPassFromEEPROM();

  bool match = true;
  for (uint8_t j = 0; j < sizeof(passphrase); j++) {
    if (passphrase[j] != atoi(&pass[j*2])) {
      match = false;
      break;
    }
  }

  return match;
}

/*
 * Check BLE Characteristic for HM-10 BLE Module and process any commands. 
 */
void checkBLECharacteristic() {
  char command[100];
  memset(command, 0, sizeof(command));

  uint8_t i = 0;
  while (bleSerial.available()) {
    command[i] = bleSerial.read();
    command[i+1] = '\0';
    i += 2;
  }
  if (i) {
    int first = atoi(command);
    int second = atoi(&command[2]);
    int third = atoi(&command[4]);
    
    /*
    bleSerial.print(first);
    bleSerial.print(second);
    bleSerial.println(third);
    */
    
    switch (first) {
      case connectionCategory:
        switch (second) {
          case connectionValidatePassCommand: //11XXXX
            if (checkValidPass(&command[4]))
              bleSerial.print(0);
            else
              bleSerial.print(1);

            break;
          case connectionSetPassCommand: //12XXXXYYYY
            if (checkValidPass(&command[4])) {
              uint8_t newPassLength = 0;
              while (command[4+sizeof(passphrase)*2+newPassLength*2] != 0) {
                newPassLength++;
              }

              uint8_t oldPassLength = sizeof(passphrase);

              passphrase[newPassLength];
              for (uint8_t j = 0; j < newPassLength; j++) {
                passphrase[j] = atoi(&command[4+sizeof(oldPassLength)*2+j*2]);
              }

              bleSerial.print(0);
            }
            else
              bleSerial.print(1);


            break;
        }
      break;
      case securityCategory:
        switch (second) {
          case securityLockCommand: //21
            digitalWrite(shiftRegisterEnablePin, HIGH);
            shift.writeBit(lockPin, HIGH);
            delay(500);
            shift.writeBit(lockPin, LOW);
            delay(1000);
            if (third) { //received "211"
              shift.writeBit(lockPin, HIGH);
              delay(500);
              shift.writeBit(lockPin, LOW);
              delay(1000);
            }
            break;

          case securityUnlockCommand: //22
            digitalWrite(shiftRegisterEnablePin, HIGH);
            shift.writeBit(unlockPin, HIGH);
            delay(500);
            shift.writeBit(unlockPin, LOW);
            delay(1000);
            if (third) { //received "221"
              shift.writeBit(unlockPin, HIGH);
              delay(500);
              shift.writeBit(unlockPin, LOW);
              delay(1000);
            }
            break;

            case alarmCommand: //23
            digitalWrite(shiftRegisterEnablePin, HIGH);
            shift.writeBit(alarmPin, HIGH);
            delay(500);
            shift.writeBit(alarmPin, LOW);
            delay(1000);
            break;

          default:
            break;
        }
        break;
      default:
        break;
    }
  }
  digitalWrite(shiftRegisterEnablePin, LOW);
}

/*
 * ACP communication setup
 */
void acp_setup() {
  outp(0xff, PORTD);
  outp(0xC0, DDRD);
  
  pinMode(switchPin, OUTPUT);
  digitalWrite(switchPin, LOW);
  delay(500);
  acp_uart_init(ACP_UART_BAUD_SELECT); // Initialize the ACP USART
  delay(1);
}

/*
 * Playback inline control setup
 */
void inline_control_setup() {
  //Setup inline control pin
  digitalWrite(inlineControlPin, LOW);
  pinMode(inlineControlPin, OUTPUT);
}

/*
 * Setup HM-10 BLE Serial
 */
void ble_setup() {
  //Setup HM-10 BLE serial
  bleSerial.begin(9600);

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
}

/*
 * Setup remote lock/unlock by BLE.
 */
void remote_access_setup() {

  //Write default passphrase and header to EEPROM if invalid header
  if (!checkValidEEPROMHeader()) {
    writePassToEEPROM();
    writeHeaderToEEPROM();
  }

  ble_setup();
}

/*
 * Simulate inline control based on value of lastCommand.
 */
void inline_control_handler() {
  //The shortest duration of digitalWrite that is sensed by iPhone SE is ~60ms.
  if (lastCommand != noCommand) {
    digitalWrite(inlineControlPin, LOW);
    switch(lastCommand) {
      case playPause:
        digitalWrite(inlineControlPin, HIGH);
        delay(100);
        digitalWrite(inlineControlPin, LOW);
        break;
      case nextTrack:
        digitalWrite(inlineControlPin, HIGH);
        delay(100);
        digitalWrite(inlineControlPin, LOW);
        delay(100);
        digitalWrite(inlineControlPin, HIGH);
        delay(100);
        digitalWrite(inlineControlPin, LOW);
        break;
      case prevTrack:
        digitalWrite(inlineControlPin, HIGH);
        delay(100);
        digitalWrite(inlineControlPin, LOW);
        delay(100);
        digitalWrite(inlineControlPin, HIGH);
        delay(100);
        digitalWrite(inlineControlPin, LOW);
        delay(100);
        digitalWrite(inlineControlPin, HIGH);
        delay(100);
        digitalWrite(inlineControlPin, LOW);
        break;
      case fastForwardTrack:
        digitalWrite(inlineControlPin, HIGH);
        delay(100);
        digitalWrite(inlineControlPin, LOW);
        delay(100);
        digitalWrite(inlineControlPin, HIGH);
        break;
      case rewindTrack:
        digitalWrite(inlineControlPin, HIGH);
        delay(100);
        digitalWrite(inlineControlPin, LOW);
        delay(100);
        digitalWrite(inlineControlPin, HIGH);
        delay(100);
        digitalWrite(inlineControlPin, LOW);
        delay(100);
        digitalWrite(inlineControlPin, HIGH);
        break;
      case activateSiri:
        digitalWrite(inlineControlPin, HIGH);
        delay(2000);
        digitalWrite(inlineControlPin, LOW);
        break;
    }
    lastCommand = noCommand;
  }
}

void setup(){
  acp_setup();
  
  inline_control_setup();

  remote_access_setup();
}

void loop()
{
  //Handle ACP communication
  acp_handler();
  
  //Handle inline control
  inline_control_handler();
  
  //Check for BLE commands
  checkBLECharacteristic();
  delay(100);
}

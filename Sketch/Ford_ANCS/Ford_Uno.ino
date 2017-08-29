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


/*
//BLE Control
#include <Shifty.h>
#include <AltSoftSerial.h>
#define securityCategory 2
#define lockCommand 1
#define unlockCommand 2
#define alarmCommand 3
AltSoftSerial bleSerial(2,3);
Shifty shift;
const uint8_t lockPin = 0;
const uint8_t unlockPin = 1;
const uint8_t alarmPin = 2;
*/
const uint8_t shiftRegisterEnablePin = 17;

/*
//ANCS lib
#define ANCS_USE_APP
#define ANCS_USE_SUBTITLE
#include <lib_aci.h>
#include <SPI.h>
#include <EEPROM.h>

#include <notif.h>

#define LCD_SIZE 16

Notif notif(5,2);
ancs_notification_t* current_notif = NULL;
uint8_t title_char = 0;
uint8_t subtitle_char = 0;
uint8_t title_scroll_wait = 0;
uint8_t subtitle_scroll_wait = 0;
uint8_t start_scroll_wait = 5;
uint8_t end_scroll_wait = 5;
char wait = ' ';
unsigned long screen_update_timer = 0;
const uint8_t screen_timer_interval = 500;
boolean connected = false;

//custom chars for connectivity status
byte antenna_char[8] = {
    B00000,
    B00000,
    B00000,
    B00100,
    B00100,
    B00100,
    B01110,
};

byte connected_char[8] = {
    B00000,
    B01110,
    B10001,
    B00100,
    B00100,
    B00100,
    B01110,
};

byte disconnected_char[8] = {
    B00000,
    B01010,
    B00100,
    B01010,
    B00100,
    B00100,
    B01110,
};
*/

/*
// hd44780 with hd44780_I2Cexp i/o class
#include <Wire.h>
#include <hd44780.h> // include hd44780 library header file
#include <hd44780ioClass/hd44780_I2Cexp.h> // i/o expander/backpack class
hd44780_I2Cexp lcd; // auto detect backpack and pin mappings
*/

#include <LCD.h>
#include <LiquidCrystal_SR.h>
// constructor prototype parameter:
//  LiquidCrystal_SR lcd(DataPin, ClockPin, EnablePin);
LiquidCrystal_SR lcd(A5, A3, A4); 

/*
//LCD595
#include <LiquidCrystal595.h>    // include the library
LiquidCrystal595 lcd(4,5,A4);     // datapin, latchpin, clockpin
*/

byte one_row[8] = {
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B11111
};

byte two_row[8] = {
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B11111,
    B11111
};

byte three_row[8] = {
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B11111,
    B11111,
    B11111
};

byte four_row[8] = {
    B00000,
    B00000,
    B00000,
    B00000,
    B11111,
    B11111,
    B11111,
    B11111
};

byte five_row[8] = {
    B00000,
    B00000,
    B00000,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111
};

byte six_row[8] = {
    B00000,
    B00000,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111
};

byte seven_row[8] = {
    B00000,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111
};

byte zero_row[8] = {
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B01010
};

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
void checkBLECharacteristic() {
  char command[6];
  int i = 0;
  while (bleSerial.available()) {
    command[i] = bleSerial.read();
    command[i+1] = '\0';
    i += 2;
  }
  if (i) {
    int first = atoi(command);
    int second = atoi(&command[2]);
    int third = atoi(&command[4]);
    
    bleSerial.print(first);
    bleSerial.print(second);
    bleSerial.println(third);
    
    switch (first) {
      case securityCategory:
        switch (second) {
          case lockCommand: //21
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

          case unlockCommand: //22
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
*/

/*
void eepromWrite(int address, uint8_t value)
{
  eeprom_write_byte((unsigned char *) address, value);
}

void update_lcd() {
  if (current_notif) {

    lcd.home();

    char buffer[3];
    itoa(current_notif->category, buffer, 10);
    switch(current_notif->category) {
      case ANCS_CATEGORY_OTHER:
        lcd.print("Other");
        break;
      case ANCS_CATEGORY_INCOMING_CALL:
        lcd.print("Incoming Call");
        break;
      case ANCS_CATEGORY_MISSED_CALL:
        lcd.print("Missed Call");
        break;
      case ANCS_CATEGORY_VOICEMAIL:
        lcd.print("Voicemail");
        break;
      case ANCS_CATEGORY_SOCIAL:
        lcd.print("Social");
        break;
      case ANCS_CATEGORY_SCHEDULE:
        lcd.print("Schedule");
        break;
      case ANCS_CATEGORY_EMAIL:
        lcd.print("Email");
        break;
      case ANCS_CATEGORY_NEWS:
        lcd.print("News");
        break;
      case ANCS_CATEGORY_HEALTH_FITNESS:
        lcd.print("Health Fitness");
        break;
      case ANCS_CATEGORY_BUSINESS_FINANCE:
        lcd.print("Business Finance");
        break;
      case ANCS_CATEGORY_LOCATION:
        lcd.print("Location");
        break;
      case ANCS_CATEGORY_ENTERTAINMENT:
        lcd.print("Entertainment");
        break;
      default:
        lcd.print("Unknown ");
        lcd.print(buffer);
    }
    //lcd.print("-A");
    //lcd.print(current_notif->app);

    lcd.setCursor(0,1);
    if (strlen(current_notif->title) >= LCD_SIZE) { //Message is too long for LCD_SIZE, scroll it
      if (strlen(current_notif->title)-title_char > LCD_SIZE) {
        char title_buffer[LCD_SIZE+1];
        memset(title_buffer, 0, sizeof(title_buffer));
        memcpy(title_buffer, &(current_notif->title[title_char]), LCD_SIZE);
        lcd.print(title_buffer);

        //Pause for start_scroll_wait time before beginning to scroll
        if (title_scroll_wait < start_scroll_wait) {
          title_scroll_wait++;
        } else {
          title_char++;
        }
      } else { //showed enough so that length of remaining message length is = LCD_SIZE
        lcd.print(&(current_notif->title[title_char]));
        if (title_scroll_wait == start_scroll_wait+end_scroll_wait) { //wait for (end_scroll_wait - start_scroll_wait) * 1000ms before reseting start char position
          title_char = 0;
          title_scroll_wait = 0;
        } else {
          title_scroll_wait++;
        }
      }
    } else {
      lcd.print(&(current_notif->title[title_char]));
    }


    lcd.setCursor(0,2);
    if (strlen(current_notif->subtitle) >= LCD_SIZE) { //Message is too long for LCD_SIZE, scroll it
      if (strlen(current_notif->subtitle)-subtitle_char > LCD_SIZE) {
        char subtitle_buffer[LCD_SIZE+1];
        memset(subtitle_buffer, 0, sizeof(subtitle_buffer));
        memcpy(subtitle_buffer, &(current_notif->subtitle[subtitle_char]), LCD_SIZE);
        lcd.print(subtitle_buffer);

        //Pause for start_scroll_wait time before beginning to scroll
        if (subtitle_scroll_wait < start_scroll_wait) {
          subtitle_scroll_wait++;
        } else {
          subtitle_char++;
        }
      } else { //showed enough so that length of remaining message length is = LCD_SIZE
        lcd.print(&(current_notif->subtitle[subtitle_char]));
        if (subtitle_scroll_wait == start_scroll_wait+end_scroll_wait) { //wait for (end_scroll_wait - start_scroll_wait) * 1000ms before reseting start char position
          subtitle_char = 0;
          subtitle_scroll_wait = 0;
        } else {
          subtitle_scroll_wait++;
        }
      }
    } else {
      lcd.print(&(current_notif->subtitle[subtitle_char]));
    } 
  }

  //Update connectivity status
  lcd.setCursor(15,0);
    lcd.print(wait);
    if (wait == 0)
    {
        if (connected){
            wait = 1;
        }else {
            wait = 2;
        }
    } else {
        wait=0;
    }

  screen_update_timer = millis();
}

void ancs_notifications(ancs_notification_t* notif) {
  current_notif = notif;
  title_char = 0;
  subtitle_char = 0;

  lcd.clear();
}

void ancs_connected() {
    connected = true;
}
void ancs_disconnected() {
    connected = false;
}

void ancs_reset() {
    connected = false;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(" Bond Cleared ");
    lcd.setCursor(0,1);
    lcd.print("Please Reset");
}
*/

void setup(){
  //If things get really crazy, uncomment this line. It wipes the saved EEPROM information for the Nordic chip. Good to do this if the services.h file gets updated.
  //After it is wiped, comment and reupload.
  //eepromWrite(0, 0xFF);



  

  
  outp(0xff, PORTD);
  outp(0xC0, DDRD);
  
  
  pinMode(switchPin, OUTPUT);
  digitalWrite(switchPin, LOW);
  delay(500);
  acp_uart_init(ACP_UART_BAUD_SELECT); // Initialize the ACP USART
  delay(1);
  

  
  //Setup inline control pin
  digitalWrite(inlineControlPin, LOW);
  pinMode(inlineControlPin, OUTPUT);
  
  

  uint8_t i = 0;

  lcd.begin(16,4);             // 16 characters, 2 rows
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Wow. 3 hello");
  lcd.setCursor(1,1);
  lcd.print("Fabulous");
  
  lcd.createChar(1, one_row);
  lcd.createChar(2, two_row);
  lcd.createChar(3, three_row);
  lcd.createChar(4, four_row);
  lcd.createChar(5, five_row);
  lcd.createChar(6, six_row);
  lcd.createChar(7, seven_row);
  lcd.createChar(0, zero_row);
  
  

  
  /*
  //Setup ANCS lib
  lcd.createChar(0, antenna_char);
  lcd.createChar(1, connected_char);
  lcd.createChar(2, disconnected_char);
  notif.setup();
  notif.set_notification_callback_handle(ancs_notifications);
  notif.set_connect_callback_handle(ancs_connected);
  notif.set_disconnect_callback_handle(ancs_disconnected);
  notif.set_reset_callback_handle(ancs_reset);
  */

  /*
  //Setup HM-10 BLE serial
  bleSerial.begin(9600);

  //Setup shift register
  digitalWrite(shiftRegisterEnablePin, LOW);
  pinMode(shiftRegisterEnablePin, OUTPUT);
  shift.setBitCount(8);
  shift.setPins(16, 14, 15); //data,clock,latch
  shift.batchWriteBegin();
  for (i = 0; i < 8; i++)
    shift.writeBit(i, LOW);
  shift.batchWriteEnd();
  */
  //digitalWrite(shiftRegisterEnablePin, LOW);
  //pinMode(shiftRegisterEnablePin, OUTPUT);
  
  
  //LCD Debug
  //Debug for reading acp array
  char buffer[3];
  itoa(sizeof(acp_rx)/sizeof(uint8_t), buffer, 10);
  //lcd.write(buffer);
  
  acp_rx[0] = 0x11;
  acp_rx[1] = 0x22;
  acp_rx[2] = 0x33;
  acp_rx[3] = 0x44;
  acp_rx[4] = 0x55;
  acp_rx[5] = 0x66;
  acp_rx[6] = 0x77;
  acp_rx[7] = 0x88;
  acp_rx[8] = 0x99;
  acp_rx[9] = 0xaa;
  acp_rx[10] = 0xbb;
  acp_rx[11] = 0xcc;
  
  /*
  //test bar graph on single index i
  lcd.clear();
  i = 11;
  long original = acp_rx[i];
  writeGraphForIndexForValue(i-4, original);
  lcd.home();
  lcd.print(original);
  lcd.print(" ");
  long scaled = map(original, 0, 255, 0, 32);
  lcd.print(scaled);
  */

  
  //test bar graph on indices 4-11
  lcd.clear();
  i = 4;
  for (i = 4; i < sizeof(acp_rx)/sizeof(uint8_t); i++) {
    long original = acp_rx[i];
    writeGraphForIndexForValue(i-4, original);
  }
  
  memset(acp_rx, 0, (sizeof(uint8_t))*(sizeof(acp_rx)));
  

/*
  //print test acp array
  for (i = 0; i < sizeof(acp_rx)/sizeof(uint8_t); i++) {
    char buffer[3];
    int16_t expanded = (int16_t)acp_rx[i];
    itoa((int16_t)expanded, buffer, 16);
    lcd.setCursor(i*2, 0);
    lcd.write(buffer);
  }
  memset(acp_rx, 0, (sizeof(uint8_t))*(sizeof(acp_rx)));
  for (i = 0; i < sizeof(acp_rx)/sizeof(uint8_t); i++) {
    char buffer[3];
    int16_t expanded = (int16_t)acp_rx[i];
    itoa((int16_t)expanded, buffer, 16);
    lcd.setCursor(i*2, 0);
    lcd.write(buffer);
  }

  */
  
  /*
  //test custom characters
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(255);
  lcd.setCursor(0, 1);
  lcd.write(255);
  lcd.write(1);
  lcd.write(2);
  lcd.write(3);
  lcd.write(4);
  lcd.write(5);
  lcd.write(6);
  lcd.write(7);
  lcd.write(8);
  */

}

void loop()
{
  
  acp_handler();
  
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
  

  /*
  if((millis() - screen_update_timer) > screen_timer_interval) {
    update_lcd();
  }
  notif.ReadNotifications();
  */

  //checkBLECharacteristic();
  //delay(100);
}

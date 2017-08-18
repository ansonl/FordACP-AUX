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
    B10101
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
uint8_t acp_rx_old[12];

void setup(){

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
  
  //LCD Debug
  //Debug for reading acp array
  char buffer[3];
  itoa(sizeof(acp_rx)/sizeof(uint8_t), buffer, 10);
  //lcd.write(buffer);
  
  /*
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
  */

  acp_rx[4] = 0x00;
  acp_rx[5] = 0x2a;
  acp_rx[6] = 0x54;
  acp_rx[7] = 0x7f;
  acp_rx[8] = 0x80;
  acp_rx[9] = 0xaa;
  acp_rx[10] = 0xee;
  acp_rx[11] = 0xff;
  
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
  memset(acp_rx_old, 0, (sizeof(uint8_t))*(sizeof(acp_rx_old)));
  

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
}

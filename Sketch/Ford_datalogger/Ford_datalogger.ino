//SD data logging
#include <SPI.h>
#include <SD.h>
//#include "libraries/SD/src/SD.h" //Modified SD library with all Serial functions removed. SerialPrint_P() in utility/SdFatUtil.h becomes empty.
const int chipSelect = 10;

//Accelerometer ADXL335
const int xInput = A0;
const int yInput = A1;
const int zInput = A2;
const int sampleSize = 3;

#define LCD_SIZE 16

/*
// hd44780 with hd44780_I2Cexp i/o class
//#include <Wire.h>
#include <hd44780.h> // include hd44780 library header file
#include <hd44780ioClass/hd44780_I2Cexp.h> // i/o expander/backpack class
hd44780_I2Cexp lcd; // auto detect backpack and pin mappings
*/

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

int ReadAxis(int axisPin)
{
  long reading = 0;
  analogRead(axisPin);
  delay(1);
  for (int i = 0; i < sampleSize; i++)
  {
    reading += analogRead(axisPin);
  }
  return reading/sampleSize;
}

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

  /*
  //Setup LCD
  lcd.begin(16,4);
  lcd.clear();
  //lcd.cursor();
  */
  
  /*
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

  //SD data logging
  //lcd.clear();
  //lcd.write("Initializing SD card...");
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    //lcd.write("Card failed, or not present");
    return;
  }
  //lcd.clear();
  //lcd.write("card initialized.");

  //ADXL335 accelerometer setup
  analogReference(EXTERNAL);
}

void loop()
{
  acp_handler();

  /*
   * The shortest duration of digitalWrite that is sensed by iPhone SE is ~60ms.
   */
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

/* 
 * Ford Head Unit to Bluetooth Interface V 1.0
 *
 * The code in this file is intended to be uploaded to an Arduino Mega2560
 * used in combination with a specialized shield that contains the hardware
 * needed to interface with a Ford head unit and bluetooth module.
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
 * Bluetooth Module: OVC3860 on one of two breakout boards.
 *                   The XS3868 or BLK-MD-SPK-B.
 *
 * The code for the ACP protocol was originally written by Andrew Hammond for his Yampp
 * and was ported over to the Arduino platform by Krzysztof Pintscher. Please see the 
 * ACP and CD headers for more info.
 *
 **************************************************************************************
 * Revisions:
 *
 * 2014-11-30 version 1.0 - public release
 *
 **************************************************************************************
 * Requirements:
 *
 * -LiquidCrystal library
 * -TimerOne library
 * -TimerThree library
 *
 **************************************************************************************
 * TODO:
 *
 * -remove lcd code unless it proves useful for something other than debug
 * -add code to automatically change and save the name of the OVC (Ford Bluetooth?)
 *
 */
 
#include <LiquidCrystal.h>
#include <util/delay.h>
#include <TimerOne.h>
#include <TimerThree.h>

typedef unsigned char u08;
typedef unsigned short u16;
typedef unsigned long  u32;

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
#define AT_UART_BAUD_RATE 	115200
#define AT_UART_BAUD_SELECT ((u32)((u32)16000000/(AT_UART_BAUD_RATE*8L)-1))
#define ACP_LISTEN 0
#define ACP_SENDACK 1
#define ACP_MSGREADY 2
#define ACP_WAITACK 3
#define ACP_SENDING 4 
#define HU_EV_STOP 1
#define HU_EV_CANCEL_VOICE_CALL 9

const int switchPin = 28; // Pin 28 connected to pins 2 & 3 (RE/DE) on MAX485
const int audioOnPin = 29; // Pin 29 connected to "Audio On" pin from head unit
const int wakePin = 8; // pin 8 connected to MFB pin of OVC
int audioOnState = 0;
int lastAudioPinState = 2;

LiquidCrystal lcd(2, 3, 4, 9, 10, 11, 12);
int c = 0; // used for lcd debug

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
boolean vCall = false;
boolean rewindState = false;
boolean ffState = false;
boolean callStatus = false;
boolean wPlayTime = false;
int currentTrack = 1;
boolean reset_timer = false;

uint8_t at_tx[8];
uint8_t at_rx[9];
uint8_t at_rxindex = 0;
u08 at_checksum;
u08 at_txsize = 8;
u08 at_rxsize = 9;

void setup(){
  
  lcd.begin(16,2);//specify screen dimensions
  lcd.clear();//clear the screen
  lcd.setCursor(0,0); // set cursor to col 0 row 0
  lcd.print("OVC3868");// print from cursor location TODO: get name of blutooth module (see above)
  
  pinMode(wakePin, OUTPUT);
  digitalWrite(wakePin, LOW);
  pinMode(audioOnPin, INPUT);
  outp(0xff, PORTD);
  outp(0xC0, DDRD);
  pinMode(switchPin, OUTPUT);
  digitalWrite(switchPin, LOW);
  delay(500);
  acp_uart_init(ACP_UART_BAUD_SELECT); // Initialize the ACP USART
  at_uart_init(AT_UART_BAUD_SELECT); // Initialize the AT USART
  delay(1);
}

void disconnectBluetooth() { // messages required to disconnect the OVC
     at_process(16); // Disconnect Audio Source
     at_process(3);  // Disconnect hshf
     at_process(6);  // Enter Pairing
}

void wakeOVC() // 16.5ms pulse required to wake the OVC
{
  digitalWrite(wakePin, HIGH); 
  delay(1100);
  digitalWrite(wakePin, LOW);
}

void loop()
{
  audioOnState = digitalRead(audioOnPin); // monitor the "Audio On" signal
  if(audioOnState != lastAudioPinState){
   if (audioOnState == 0) {
     Timer1.detachInterrupt(); // stop sending ACP messages
     Timer3.initialize(3000000); // start sending disconnect messages to OVC
     Timer3.attachInterrupt(disconnectBluetooth);
   }
   if (audioOnState == 1) {
    Timer3.detachInterrupt(); // stop sending disconnect messages to OVC
   }
  } 
  lastAudioPinState = audioOnState; 
  acp_handler();  
}


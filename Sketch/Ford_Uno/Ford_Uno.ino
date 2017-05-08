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
 * The code for the ACP protocol was originally written by Andrew Hammond for his Yampp
 * and was ported over to the Arduino platform by Krzysztof Pintscher. Please see the 
 * ACP and CD headers for more info.
 *
 * Version 1.1 has been tested to be working with 2007 Ford Escape Hybrid audio system.
 *
 **************************************************************************************
 * Revisions:
 *
 * 2014-11-30 version 1.0 - public release
 * 2017-05-07 version 1.1 - Removed Bluetooth functionality and editted ports for use with Arduino UNO. -AL
 *
 **************************************************************************************
 * Requirements:
 *
 * -TimerOne library
 *
 */
#include <util/delay.h>
#include <TimerOne.h>

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
#define ACP_LISTEN 0
#define ACP_SENDACK 1
#define ACP_MSGREADY 2
#define ACP_WAITACK 3
#define ACP_SENDING 4

const int switchPin = 8; // Pin 8 connected to pins 2 & 3 (RE/DE) on MAX485

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
boolean wPlayTime = false;
int currentTrack = 1;
boolean reset_timer = false;

void setup(){
  outp(0xff, PORTD);
  outp(0xC0, DDRD);
  pinMode(switchPin, OUTPUT);
  digitalWrite(switchPin, LOW);
  delay(500);
  acp_uart_init(ACP_UART_BAUD_SELECT); // Initialize the ACP USART
  delay(1);
}

void loop()
{
  acp_handler();  
}


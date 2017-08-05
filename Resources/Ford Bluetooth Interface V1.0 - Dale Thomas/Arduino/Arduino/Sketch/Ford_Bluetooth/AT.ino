/* 
 * This code is designed for use with an Atmel AtMega2560 and is used 
 * to send and receive AT commands from an OVC3860 Bluetooth chip.
 *
 * Written by Dale Thomas <dalethomas@me.com>
 *
 ******************************************************************************
 * Revisions:
 *
 * 2014-11-30 version 1.0 - public release
 *
 */
void at_uart_init(unsigned short baud) // set up the USART manually because 
{                                      // the other USART's will be set differently
  cli();
  outp((u08)(baud>>8), UBRR2H);                  
  outp((u08)baud, UBRR2L);
  sbi(UCSR2A, U2X2); // Double the USART transmission speed
  sbi(UCSR2B, RXEN2); // Receiver enable
  sbi(UCSR2B, TXEN2); // Transmitter enable
  sbi(UCSR2B, RXCIE2); // RX complete interrupt enable 
  cbi(UCSR2B, UDRIE2); // USART data register empty interrupt enable
  sei();
 }

SIGNAL(USART2_RX_vect){ // this is automatically called when USART2 receives
  uint8_t ch = inp(UDR2);
  at_rx[at_rxindex++] = ch;
  if (at_rxindex >= 6){
    at_handler();
    at_rxindex = 0; // Clear the receive index
    cbi(UCSR2B, RXEN2); // Receiver disable. this will flush the recieve buffer
    sbi(UCSR2B, RXEN2); // Receiver enable 
  } 
  return;  
}

void at_handler(){ // this takes the recieved hex from USART2 and makes use of it
   u08 M;
   for(M=0; M<=at_rxindex; M++){
     if(at_rx[M] == 77){
       switch (at_rx[M + 1]){
         case 65: // ASCII "MA" response indication from the Stop notion
              return;
         case 77: // ASCII "MM" response indication from name query
              return;
       }
     }
       else if(at_rx[M] == 73){
              switch (at_rx[M + 1]){
                     case 65: // ASCII "IA" response indication from the Disconnect hshf notion
                          return;
                     case 73: // ASCII "II" response indication from the Enter Pairing notion
                          return;
                     case 86: // ASCII "IV" response indication from the Connect hshf notion
                          return;
              }
      }
   }  
}

 char at_process(int event) // this builds the string that will be send to the OVC
{
  at_tx[0] = 0x0D; // ASCII "CR"
  at_tx[1] = 0x0A; // ASCII "LF"
  at_tx[2] = 0x41; // ASCII "A"
  at_tx[3] = 0x54; // ASCII "T"
  at_tx[4] = 0x23; // ASCII "#"   
  switch(event)
  {
    case 0: // Play/Pause Music
         at_tx[5] = 0x4D; // ASCII "M"
         at_tx[6] = 0x41; // ASCII "A"
         break;
    case 1: // Stop Music
         at_tx[5] = 0x4D; // ASCII "M"
         at_tx[6] = 0x43; // ASCII "C"
         break;
    case 2: // Connect hshf
         at_tx[5] = 0x43; // ASCII "C" 
         at_tx[6] = 0x43; // ASCII "C" 
         break;
    case 3: // Disconnect hshf
         at_tx[5] = 0x43; // ASCII "C" 
         at_tx[6] = 0x44; // ASCII "D" 
         break;
    case 4: // Forward Music
         at_tx[5] = 0x4D; // ASCII "M"
         at_tx[6] = 0x44; // ASCII "D"
         break;
    case 5: // Backward Music
         at_tx[5] = 0x4D; // ASCII "M"
         at_tx[6] = 0x45; // ASCII "E"
         break;
    case 6: // Enter Pairing
         at_tx[5] = 0x43; // ASCII "C" 
         at_tx[6] = 0x41; // ASCII "A" 
         break;
    case 7: // Exit Pairing
         at_tx[5] = 0x43; // ASCII "C" 
         at_tx[6] = 0x42; // ASCII "B" 
         break;
    case 8: // Answer Call
         at_tx[5] = 0x43; // ASCII "C" 
         at_tx[6] = 0x45; // ASCII "E" 
         break;
    case 9: // Reject Call
         at_tx[5] = 0x43; // ASCII "C" 
         at_tx[6] = 0x46; // ASCII "F" 
         break;
    case 10: // End Call
         at_tx[5] = 0x43; // ASCII "C" 
         at_tx[6] = 0x47; // ASCII "G" 
         break; 
    case 11: // Voice Dial (Siri)
         at_tx[5] = 0x43; // ASCII "C" 
         at_tx[6] = 0x49; // ASCII "I" 
         break;
    case 12: // Cancel Voice Dial (Siri)
         at_tx[5] = 0x43; // ASCII "C" 
         at_tx[6] = 0x4A; // ASCII "J" 
         break;
    case 13: // Mute/Unmute Mic
         at_tx[5] = 0x43; // ASCII "C" 
         at_tx[6] = 0x4D; // ASCII "M" 
         break;
    case 14: // Reset
         at_tx[5] = 0x43; // ASCII "C" 
         at_tx[6] = 0x5A; // ASCII "Z" 
         break;
    case 15: // Connect to AV Source
         at_tx[5] = 0x4D; // ASCII "M" 
         at_tx[6] = 0x49; // ASCII "I" 
         break;
    case 16: // Disconnect from AV Source
         at_tx[5] = 0x4D; // ASCII "M" 
         at_tx[6] = 0x4A; // ASCII "J" 
         break;
    case 17: // Start Fast Forward
         at_tx[5] = 0x4D; // ASCII "M" 
         at_tx[6] = 0x52; // ASCII "R" 
         break;
    case 18: // Start Rewind
         at_tx[5] = 0x4D; // ASCII "M" 
         at_tx[6] = 0x53; // ASCII "S" 
         break;
    case 19: // Stop Fast Forward / Rewind
         at_tx[5] = 0x4D; // ASCII "M" 
         at_tx[6] = 0x54; // ASCII "T" 
         break;
    case 20: // Power Off OOL
         at_tx[5] = 0x56; // ASCII "V" 
         at_tx[6] = 0x58; // ASCII "X" 
         break;
    case 21: // Enable PowerOn Auto Connection
         at_tx[5] = 0x4D; // ASCII "M" 
         at_tx[6] = 0x47; // ASCII "G" 
         break;
    case 22: // Disable PowerOn Auto Connection
         at_tx[5] = 0x4D; // ASCII "M" 
         at_tx[6] = 0x48; // ASCII "H" 
         break;
    case 23: // Query status
         at_tx[5] = 0x43; // ASCII "C" 
         at_tx[6] = 0x59; // ASCII "Y" 
         break;
    case 24: // Increase Volume
         at_tx[5] = 0x56; // ASCII "V" 
         at_tx[6] = 0x55; // ASCII "U" 
         break; 
     case 25: // Get Name
         at_tx[5] = 0x4D; // ASCII "M" 
         at_tx[6] = 0x4D; // ASCII "M" 
         break;        
  }
    at_tx[7] = 0x0D; // ASCII "CR"
    at_tx[8] = 0x0A; // ASCII "LF" 
  at_sendmsg();
}

 void at_sendmsg(void) // this handles the actual mechanics of getting a msg sent
{
   if (c >=15) // these next few lines (179 - 188) are for debug. they print out 
   {           // the sent AT commands across the bottom of the lcd.
     c = 0;
   }
   lcd.cursor();
   lcd.setCursor(c,1); // set cursor to col 0 row 0
   lcd.write(at_tx[5]);// print from cursor location
   lcd.setCursor(c + 1,1); // set cursor to col 0 row 0
   lcd.write(at_tx[6]);// print from cursor location
   c = c + 2;
  
  u08 i;
   cbi(UCSR1B, RXCIE1); // "clear bit" this clears the interrupt on USART1
                        // let's try this to keep USART1 from interrupting coms
                        // from the bluetooth module                      
  for(i=0; i <= at_txsize; i++)
  { 
    while(!(inp(UCSR2A) & _BV(UDRIE2))); // wait till tx buffer empty
    if(i==at_txsize) 
    {
      sbi(UCSR2A, TXC2); // USART transmit complete
    }
    outp(at_tx[i], UDR2);
  }
  while(!(inp(UCSR2A) & _BV(TXC2))); // wait till last character tx complete
  sbi(UCSR1B, RXCIE1); // "set bit" this sets the interrupt on USART1
                       // let's try this to keep USART1 from interrupting coms
                       // from the bluetooth module
}

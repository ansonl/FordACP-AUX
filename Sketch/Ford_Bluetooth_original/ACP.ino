/*
 * Original code for Yampp -  Andrew Hammond <andyh@mindcode.co.uk>
 *
 * For Arduino Mega 2560 - Krzysztof Pintscher <niou.ns@gmail.com>
 * Full schematics and connection by Krzysztof Pintscher
 * please keep our credits in header :)
 *
 * Integration with AT Commands for OVC3868 - Dale Thomas <dalethomas@me.com>
 *
 ******************************************************************
 * Revisions:
 *
 * 2013-12-17 version 0.9 - public relase
 * 2014-11-30 version 1.0 - Annotations and integration with AT file -DT
 *
 */
void acp_uart_init(unsigned short baud)
{
  cli();
  outp((u08)(baud>>8), UBRR1H);                  
  outp((u08)baud, UBRR1L);
  sbi(UCSR1B, RXEN1);  // Receiver enable
  sbi(UCSR1B, TXEN1);  // Transmitter enable
  sbi(UCSR1B, RXCIE1); // RX complete interrupt enable
  cbi(UCSR1B, UDRIE1); // USART data register empty complete enable
  sbi(UCSR1B, UCSZ12); // Character size n (setting this to one enables 9-bit character data)
  sei();
 }


SIGNAL(USART1_RX_vect)
{
  u08 eod = (inp(UCSR1B) & _BV(RXB81));
  uint8_t ch = inp(UDR1); 
  
  if (acp_status != ACP_LISTEN)return; // ignore incoming msgs if busy processing
  if(!eod)acp_checksum += ch;
  acp_rx[acp_rxindex++] = ch;
  if(acp_rxindex>12)acp_reset();
  else if (eod)	
  {
    if (acp_checksum == ch)      // Valid ACP message - send ack
    {              
      acp_status=ACP_SENDACK;
      acp_handler();
    }
    else acp_reset();		// Abort message
  }
  return; 
}


 void acp_txenable(boolean enable)
{
  if (enable)PORTA |= (1<<PA6); //sets a high state on pin PD4
  else PORTA &= ~(1<<PA6); //sets the low state on pin PD4
  asm volatile("nop");
}


void acp_sendack(void)
{
  while(!(inp(PORTD) & 0x01)) // Wait till RX line goes high (idle)
  cbi(UCSR1B, RXCIE1);
  acp_txenable(true);  
  sbi(UCSR1A, TXC1); // Set TXC to clear it!!
  cbi(UCSR1B, TXB81);
  outp(0x06, UDR1);
  while(!(inp(UCSR1A) & _BV(TXC1))); // wait till buffer clear
  acp_txenable(false);
}


void acp_reset(void)
{
  acp_retries=0;
  acp_timeout=0;
  acp_checksum=0;
  acp_rxindex=0;
  acp_txindex=0;
  acp_txsize=0;
  acp_status=ACP_LISTEN;
}


void acp_sendmsg(void)
{
  u08 i;
  while(!(inp(PORTD) & 0x01)) // Wait till RX line goes high (idle)
  cbi(UCSR1B, RXCIE1);
  acp_txenable(true);
  _delay_us(104*17);
  for(i=0; i <= acp_txsize; i++)
  {
    while(!(inp(UCSR1A) & _BV(UDRIE1))); // wait till tx buffer empty
    cbi(UCSR1B, TXB81);
    if(i==acp_txsize) 
    {
      sbi(UCSR1A, TXC1);   // set TXC to clear it!
      sbi(UCSR1B, TXB81);  // set 9th bit at end of message
    }
    outp(acp_tx[i], UDR1);
  }
  while(!(inp(UCSR1A) & _BV(TXC1))); // wait till last character tx complete
  acp_txenable(false); // switch back to rx mode immediately
  acp_status = ACP_WAITACK;	// wait for ACK
  sbi(UCSR1B, RXCIE1);
}


void acp_handler()
{
  if (acp_status == ACP_LISTEN)
  {
  if(++acp_ltimeout == 1000)acp_ltimeout=0; 
  }
  if (acp_status == ACP_SENDACK)
  { 
    acp_sendack();
    acp_status=ACP_MSGREADY;
  }
  else if (acp_status == ACP_WAITACK)acp_reset(); // HU does not seem to return an ACK
  if (acp_status == ACP_MSGREADY)
  {
    acp_status = ACP_SENDING;
    acp_process(); // Process message		
  }
  else if (acp_status == ACP_SENDING)
  {
    acp_timeout++;
    if(!acp_timeout)
    {
      acp_reset();
      acp_txenable(false);
    }		
  }
}


void acp_process(void)
{	
  acp_timeout = 0;
  acp_tx[0] = 0x71; // medium/low priority
  acp_tx[1] = 0x9b;
  acp_tx[2] = 0x82; // message from "CD Changer" (bluetooth interface)
  acp_tx[3] = acp_rx[3];

  if (acp_rx[2] == 0x80)  // Message from Head Unit
  {
    if(acp_rx[1]==0x9a || acp_rx[1]==0x9b) // CD Changer functional address 
    {
      switch (acp_rx[3])
      {
      case 0xC8:		// Handshake 3 - CD Changer now recognized
        acp_tx[4] = acp_rx[4];
        acp_chksum_send(5);
        break;

      case 0xFC:		// Handshake 2
        acp_tx[4] = acp_rx[4];
        acp_chksum_send(5);
        break;

      case 0xE0:		// Handshake 1
        acp_tx[4] = 0x04;
        acp_chksum_send(5);
        break;

      case 0xFF:		// Current disc status request - responses are
        acp_tx[4]= 0x00;
                                // 00 - Disc OK
                                // 01 - No disc in current slot
                                // 02 - No discs
                                // 03 - Check disc in current slot
                                // 04 - Check all discs!    
        acp_chksum_send(5);
        Timer1.initialize(1000000);
        Timer1.attachInterrupt(PlayTime);        
        break;
        
      case 0x42:		 // [<- Tune]
           switch(callStatus){
           case false:
           callStatus = true;
           at_process(8);       // Answer Call
           break;
           case true:
           callStatus = false;
           at_process(10);      // End Call
           break;
          }
           acp_chksum_send(5);
           break;
           
      case 0xC2:		// [Tune ->]
           at_process(9);       // Reject Call
           acp_chksum_send(5);
           break;
           
      case 0xD0:
        if (acp_rx[1] == 0x9a) // Command to change disc
        {
          //u08 disc = plist_change(acp_rx[4]);
          acp_tx[4] = 1 & 0x7F; 
          acp_chksum_send(5);
          //if(disc & 0x80)
          //acp_nodisc();
          break;
        } 
        else	                 // Request current disc
        {
          //acp_tx[4] = get_disc();
        }
        if (acp_rx[3]!=0xD0) acp_chksum_send(5);
        break;
        
      case 0xC1:		// Command
        acp_mode = acp_rx[4];
        switch(acp_mode){
          case 0x00: // Switch from CD ie. FM, AM, Tape or power button. 
                     // there is no opposite to this. turning power back on only sends "play"
               at_process(16); // Disconnect Audio Source
               at_process(3);  // Disconnect hshf
               at_process(6);  // Enter Pairing
          break;
          case 0x40: // Switch to CD ie. Audio On (vehichle is turned on) or CD button
               at_process(2); // Connect hshf
               at_process(15); // Connect Audio Source
          break;
          case 0x41: // Scan Button
          at_process(13); // Mute/Unmute Mic
          break;
          case 0x42: // FF Button
               switch(ffState){
               case false:
               ffState = true;
               at_process(17); // Start Fast Forward
               break;
               case true:
               ffState = false;
               at_process(19); // Stop Fast Forward / Rewind
               break;
               }
          break;
          case 0x44: // Rew Button
               switch(rewindState){
               case false:
               rewindState = true;
               at_process(18); // Start Rewind
               break;
               case true:
               rewindState = false;
               at_process(19); // Stop Fast Forward / Rewind
               break;
               }
          break;
          case 0x50: // Shuffle Button
               switch(vCall){
                     case false:
                     vCall = true;
                     at_process(11); // Voice Dial (Siri)
                     break;
                     case true:
                     vCall = false;
                     at_process(12); // Cancel Voice Dial (Siri)
                     break;
              }
          break;
          case 0x60: // Comp Button
          at_process(0); // Play/Pause Music
          break;
        }
        acp_mode = (acp_mode & 0x40); 
        acp_tx[4] = acp_mode;
        acp_chksum_send(5);
        break;
        
      case 0xC3:
        change_track(true);	// Next Track
        acp_tx[4] = BCD(currentTrack);
        acp_chksum_send(5);
        break;
        
      case 0x43:
        change_track(false);  // Prev Track
        acp_tx[4] = BCD(currentTrack);
        acp_chksum_send(5);
        break;
        
      default: acp_reset(); // unknown - ignore  
      }
    }
    else acp_reset(); // Ignore all other acp messages
  }
}


void acp_chksum_send(unsigned char buffercount)
{
  u08 i;
  u08 checksum = 0;
  for(i=0;i<buffercount;i++) checksum += acp_tx[i];
  acp_txsize = buffercount;
  acp_tx[acp_txsize] = checksum;
  acp_sendmsg();
}

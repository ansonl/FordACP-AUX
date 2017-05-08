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
 * 2017-05-07 version 1.1 - Removed Bluetooth functionality and editted ports for use with Arduino UNO. -AL
 *
 */
void acp_uart_init(unsigned short baud)
{
  cli();
  outp((u08)(baud>>8), UBRR0H);                  
  outp((u08)baud, UBRR0L);
  sbi(UCSR0B, RXEN0);  // Receiver enable
  sbi(UCSR0B, TXEN0);  // Transmitter enable
  sbi(UCSR0B, RXCIE0); // RX complete interrupt enable
  cbi(UCSR0B, UDRIE0); // USART data register empty complete enable
  sbi(UCSR0B, UCSZ02); // Character size n (setting this to one enables 9-bit character data)
  sei();
 }


SIGNAL(USART_RX_vect)
{
  u08 eod = (inp(UCSR0B) & _BV(RXB80));
  uint8_t ch = inp(UDR0); 
  
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
    else {
      acp_reset();    // Abort message
    }
  }
  return; 
}


 void acp_txenable(boolean enable)
{
  if (enable)PORTB |= (1<<PB0); //sets a high state on pin PD4
  else PORTB &= ~(1<<PB0); //sets the low state on pin PD4
  asm volatile("nop");
}


void acp_sendack(void)
{
  while(!(inp(PORTD) & 0x01)) // Wait till RX line goes high (idle)
  cbi(UCSR0B, RXCIE0);
  acp_txenable(true);  
  sbi(UCSR0A, TXC0); // Set TXC to clear it!!
  cbi(UCSR0B, TXB80);
  outp(0x06, UDR0);
  while(!(inp(UCSR0A) & _BV(TXC0))); // wait till buffer clear
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
  cbi(UCSR0B, RXCIE0);
  acp_txenable(true);
  _delay_us(104*17);
  for(i=0; i <= acp_txsize; i++)
  {
    while(!(inp(UCSR0A) & _BV(UDRIE0))); // wait till tx buffer empty
    cbi(UCSR0B, TXB80);
    if(i==acp_txsize) 
    {
      sbi(UCSR0A, TXC0);   // set TXC to clear it!
      sbi(UCSR0B, TXB80);  // set 9th bit at end of message
    }
    outp(acp_tx[i], UDR0);
  }
  while(!(inp(UCSR0A) & _BV(TXC0))); // wait till last character tx complete
  acp_txenable(false); // switch back to rx mode immediately
  acp_status = ACP_WAITACK; // wait for ACK
  sbi(UCSR0B, RXCIE0);
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
      case 0xC8:    // Handshake 3 - CD Changer now recognized
        acp_tx[4] = acp_rx[4];
        acp_chksum_send(5);
        break;

      case 0xFC:    // Handshake 2
        acp_tx[4] = acp_rx[4];
        acp_chksum_send(5);
        break;

      case 0xE0:    // Handshake 1
        acp_tx[4] = 0x04;
        acp_chksum_send(5);
        break;

      case 0xFF:    // Current disc status request - responses are
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
           
      case 0xC2:
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
        else                   // Request current disc
        {
          //acp_tx[4] = get_disc();
        }
        if (acp_rx[3]!=0xD0) acp_chksum_send(5);
        break;
        
      case 0xC1:    // Command
        acp_mode = acp_rx[4];
        
        acp_mode = (acp_mode & 0x40); 
        acp_tx[4] = acp_mode;
        acp_chksum_send(5);
        break;
        
      case 0xC3:
        change_track(true); // Next Track
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

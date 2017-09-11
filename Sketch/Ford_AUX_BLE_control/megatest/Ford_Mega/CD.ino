/*
 * Original code for Yampp -  Andrew Hammond <andyh@mindcode.co.uk>
 *
 * For Arduino Mega 2560 - Krzysztof Pintscher <niou.ns@gmail.com>
 * Full schematics and connection by Krzysztof Pintscher
 * please keep our credits in header :)
 *
 * Integration with AT Commands for OVC3868 - Dale Thomas <dalethomas@me.com>
 * Various annotations and improvements- Dale Thomas
 *
 ******************************************************************
 * Revisions:
 *
 * 2013-12-17 version 0.9 - public relase
 * 2017-05-07 version 1.1 - Removed Bluetooth functionality. - Anson Liu (ansonliu.com)
 *
 */
void PlayTime(){
  wPlayTime++;
  acp_displaytime();
}

void acp_displaytime()
{
  if(reset_timer == true){
    wPlayTime = 0;
    reset_timer = false; 
  }
  acp_tx[0] = 0x71 ;    
  acp_tx[1] = 0x9b ;
  acp_tx[2] = 0x82 ;
  acp_tx[3] = 0xd0 ;
  acp_tx[4] = BCD(1);
  acp_tx[5] = BCD(currentTrack) ; //Track
  acp_tx[6] = BCD(wPlayTime/60); 
  acp_tx[7] = _BV(7) | BCD (wPlayTime % 60); 
  acp_chksum_send(8);
}

uint8_t BCD(unsigned char val)
{
  return ((val/10)*16) + (val % 10) ;
}

void acp_nodisc(void)
{
  acp_tx[0] = 0x71 ;    
  acp_tx[1] = 0x9b ;
  acp_tx[2] = 0x82 ;
  acp_tx[3] = 0xff ;
  acp_tx[4] = 0x01 ;
  acp_chksum_send(5);
}

void change_track(boolean next)
{
  if(next != true){
    if(currentTrack == 1) {
    } 
    else {
     currentTrack--;
    }
  }
  else {
    currentTrack++;
  }
  reset_timer = true;
}

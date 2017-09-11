/* 
 * Ford Head Unit to iOS AUX Interface with BLE control v1.3
 * LATEST UPDATES & PROJECT AT GITHUB.COM/ansonl/ford-acp-aux
 *
 * The code in this file is intended to be uploaded to an Arduino used in combination with a specialized shield that contains the hardware needed to interface with a Ford head unit and iOS TRRS audio port.
 *
 * Compatible Ford Head Units: Any unit that uses ACP (Automotive CommunicationProtocol) and has a connector for a CD changer.
 *
 * Info on ACP:
 * http://www.mictronics.de/projects/cdc-protocols/#FordACP
 *                             
 * Some examples are: 4050RDS, 4500, 4600RDS, 5000RDS, 5000RDS EON, 6000CD RDS, 6000 MP3, 7000RDS
 *
 * These are all head units from the '96 - '07 era.
 *
 * The units used during testing are:
 * 1W1F-18C870-DA 02 03 '99 Ford Expedition
 * Navigation Head Unit '07 Ford Escape Hybrid
 *
 * The code for the ACP protocol was originally written by Andrew Hammond for his Yampp and was ported over to the Arduino platform by Krzysztof Pintscher. Please see the ACP and CD headers for more info.
 *
 * This project has been tested to be working with 2007 Ford Escape Hybrid audio system.
 *
 **************************************************************************************
 * Revisions:
 *
 * 2014-11-30 version 1.0 - public release - Dale Thomas
 * 2017-05-07 version 1.1 - Removed Bluetooth functionality and edited ports for use with Arduino UNO. - AL
 * 2017-06-16 version 1.2 - Added iOS AUX inline control commands and timing to loop(). - Anson Liu (ansonliu.com)
 * 2017-09-09 version 1.3 - Added BLE vehicle unlock functionality when used with custom wiring. PCB design included in repository.

 * 
 */

/*
 * Entry point for program
 */
void setup() {
  acp_setup();
  
  inline_control_setup();

  remote_access_setup();
}

/*
 * Program loop function
 */
void loop() {
  //Handle ACP communication
  acp_handler();
  
  //Handle inline control
  inline_control_handler();
  
  //Check for BLE commands
  checkBLECharacteristic();
}
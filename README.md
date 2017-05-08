# Ford CD changer emulator using Arduino UNO

Add AUX audio input to your older Ford vehicle using an Arduino UNO. 

  - Adapted from Dale Thomas's [*Ford Bluetooth Interface*](http://www.instructables.com/id/Ford-Bluetooth-Interface-Control-phone-with-stock-/)
  - Uses Ford Audio Control Protocol (ACP).
  - Modified for use with Arduino Uno with **switchPin** set to pin 8.
  - Protoshield wiring diagram included. 

### Directions

  - Open **Ford_Uno.ino** located in *Sketch/Ford_Uno* and upload to Arduino UNO.
  - Wire up Arduino UNO like shown below. 

![top](https://raw.githubusercontent.com/ansonl/fordacp-aux/master/Resources/top.jpg) ![bottom](https://raw.githubusercontent.com/ansonl/fordacp-aux/master/Resources/bottom.jpg)
![uno protoboard](https://raw.githubusercontent.com/ansonl/fordacp-aux/master/Resources/uno-protoboard.png)
![side](https://raw.githubusercontent.com/ansonl/fordacp-aux/master/Resources/side.jpg) ![connected side](https://raw.githubusercontent.com/ansonl/fordacp-aux/master/Resources/connected-side.jpg)

### License

Any adaptation of this work must include references to the previous authors. 

  - Andrew Hammond
  - [Krysztof Pintscher](http://www.instructables.com/id/Ford-CD-Emulator-Arduino-Mega/)
  - [Dale Thomas](http://www.instructables.com/id/Ford-Bluetooth-Interface-Control-phone-with-stock-/)
  - Anson Liu
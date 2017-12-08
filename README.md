# Ford CD changer emulator with AUX audio and stock playback control using Arduino UNO

### **Keep the retro stereo. Bring your own tunes.**

![demo](https://raw.githubusercontent.com/ansonl/fordacp-aux/master/Resources/ford_acp_aux_demo_350w.gif)

Add AUX audio input to your older Ford vehicle using a Atmega328 (Arduino UNO) and use the stock radio head unit to control playback on your iPhone. 

## [Read about the development of this.](http://ansonliu.com/2017/09/ford-acp-cd-changer-emulator-aux-audio/)

  - Adapted from Dale Thomas's [*Ford Bluetooth Interface*](http://www.instructables.com/id/Ford-Bluetooth-Interface-Control-phone-with-stock-/) and Krysztof Pintscher's  [Ford CD Emulator](http://www.instructables.com/id/Ford-CD-Emulator-Arduino-Mega/).
  - Uses Ford Audio Control Protocol (ACP) to emulate stock 6 CD Changer.
  - Adds AUX audio input to vehicle.
  - Allows stock head unit playback control of iPhone.
  - Protoshield hand wiring diagram included in `Resources` folder. 
  - EAGLE PCB files in `eagle` folder.

### Directions
  - Open **Ford_AUX_BLE_control.ino** located in *Sketch/Ford_AUX_BLE_control* and upload to Arduino UNO.
  - Acquire parts on Bill of Materials located in *Resources/LIU_FORD_ACP_AUX_BOM.xlsx*.
  - Do one of the following
    1. Wire up Arduino UNO like shown below.
    2. Print PCB using provided EAGLE files.
  - Place the assembly in your car.

#### BLE vehicle lock/unlock.
  - Follow directions above and see [ansonl/ble-control](https://github.com/ansonl/ble-control).

![PCB top](https://raw.githubusercontent.com/ansonl/fordacp-aux/master/Resources/aux_inline_top.jpg)
![closing glovebox](https://raw.githubusercontent.com/ansonl/fordacp-aux/master/Resources/closing_glovebox.gif)

![top](https://raw.githubusercontent.com/ansonl/fordacp-aux/master/Resources/top.jpg) ![bottom](https://raw.githubusercontent.com/ansonl/fordacp-aux/master/Resources/bottom.jpg)
![uno protoboard](https://raw.githubusercontent.com/ansonl/fordacp-aux/master/Resources/uno-protoboard.png)
*Please note that pin 8 in the above diagram has been changed to pin 7 in the latest Ford_AUX_BLE code. 

![side](https://raw.githubusercontent.com/ansonl/fordacp-aux/master/Resources/side.jpg) ![connected side](https://raw.githubusercontent.com/ansonl/fordacp-aux/master/Resources/connected-side.jpg)

#### ACP Data Logging and LCD Graphing
  - Use **Ford_LCD_datagraph.ino** located in *Sketch/Ford_LCD_datagraph*.

![acp visualizer](https://raw.githubusercontent.com/ansonl/fordacp-aux/master/Resources/data_bar_animated.gif)


### License

Any adaptation of this work must comply with licenses of previous works (those in `Resources` folder) and include references to the previous authors. 

**Any work by Anson Liu in this repository is under MIT License.**

  - Andrew Hammond
  - [Krysztof Pintscher](http://www.instructables.com/id/Ford-CD-Emulator-Arduino-Mega/)
  - [Dale Thomas](http://www.instructables.com/id/Ford-Bluetooth-Interface-Control-phone-with-stock-/)
  - [Anson Liu](http://ansonliu.com)

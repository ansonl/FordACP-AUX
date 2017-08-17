// hd44780 with hd44780_I2Cexp i/o class
#include <Wire.h>
#include <hd44780.h> // include hd44780 library header file
#include <hd44780ioClass/hd44780_I2Cexp.h> // i/o expander/backpack class
hd44780_I2Cexp lcd; // auto detect backpack and pin mappings

void setup(){
  //Setup LCD
  lcd.begin(16,4);
  
  lcd.clear();
  lcd.cursor();
  
  lcd.write("hi");
}

void loop()
{
  
}

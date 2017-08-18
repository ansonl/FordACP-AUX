/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the UNO, MEGA and ZERO 
  it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN is set to
  the correct LED pin independent of which board is used.
  If you want to know what pin the on-board LED is connected to on your Arduino model, check
  the Technical Specs of your board  at https://www.arduino.cc/en/Main/Products
  
  This example code is in the public domain.

  modified 8 May 2014
  by Scott Fitzgerald
  
  modified 2 Sep 2016
  by Arturo Guadalupi
  
  modified 8 Sep 2016
  by Colby Newman
*/


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(6, OUTPUT);
  digitalWrite(6, LOW);
}

// the loop function runs over and over again forever
void loop() { 
  /*
  digitalWrite(6, LOW);
  digitalWrite(6, HIGH);
  delay(60);
  digitalWrite(6, LOW);
  delay(60);
  digitalWrite(6, HIGH);
  delay(60);
  digitalWrite(6, LOW);
  delay(5000);
  */
  /*
  //rr
  digitalWrite(6, LOW);
  digitalWrite(6, HIGH);
  delay(100);
  digitalWrite(6, LOW);
  delay(100);
  digitalWrite(6, HIGH);
  delay(100);
  digitalWrite(6, LOW);
  delay(100);
  digitalWrite(6, HIGH);
  delay(5000);
  */

  //siri
  digitalWrite(6, LOW);
  delay(5000);
  digitalWrite(6, HIGH);
  delay(2000);
  digitalWrite(6, LOW);
  
}

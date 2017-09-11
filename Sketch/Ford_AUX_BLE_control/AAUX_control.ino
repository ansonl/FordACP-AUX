enum InlineControlCommand {
  noCommand,
  cancelCommand,
  playPause,
  nextTrack,
  prevTrack,
  fastForwardTrack,
  rewindTrack,
  activateSiri
};

const uint8_t inlineControlPin = 6; // Pin 6 connected to transistor for inline control

InlineControlCommand lastCommand;

/*
 * Simulate inline control based on value of lastCommand.
 */
void inline_control_handler() {
  //The shortest duration of digitalWrite that is sensed by iPhone SE is ~60ms.
  if (lastCommand != noCommand) {
    digitalWrite(inlineControlPin, LOW);
    switch(lastCommand) {
      case playPause:
        digitalWrite(inlineControlPin, HIGH);
        delay(100);
        digitalWrite(inlineControlPin, LOW);
        break;
      case nextTrack:
        digitalWrite(inlineControlPin, HIGH);
        delay(100);
        digitalWrite(inlineControlPin, LOW);
        delay(100);
        digitalWrite(inlineControlPin, HIGH);
        delay(100);
        digitalWrite(inlineControlPin, LOW);
        break;
      case prevTrack:
        digitalWrite(inlineControlPin, HIGH);
        delay(100);
        digitalWrite(inlineControlPin, LOW);
        delay(100);
        digitalWrite(inlineControlPin, HIGH);
        delay(100);
        digitalWrite(inlineControlPin, LOW);
        delay(100);
        digitalWrite(inlineControlPin, HIGH);
        delay(100);
        digitalWrite(inlineControlPin, LOW);
        break;
      case fastForwardTrack:
        digitalWrite(inlineControlPin, HIGH);
        delay(100);
        digitalWrite(inlineControlPin, LOW);
        delay(100);
        digitalWrite(inlineControlPin, HIGH);
        break;
      case rewindTrack:
        digitalWrite(inlineControlPin, HIGH);
        delay(100);
        digitalWrite(inlineControlPin, LOW);
        delay(100);
        digitalWrite(inlineControlPin, HIGH);
        delay(100);
        digitalWrite(inlineControlPin, LOW);
        delay(100);
        digitalWrite(inlineControlPin, HIGH);
        break;
      case activateSiri:
        digitalWrite(inlineControlPin, HIGH);
        delay(2000);
        digitalWrite(inlineControlPin, LOW);
        break;
    }
    lastCommand = noCommand;
  }
}

/*
 * Playback inline control setup
 */
void inline_control_setup() {
  //Setup inline control pin
  digitalWrite(inlineControlPin, LOW);
  pinMode(inlineControlPin, OUTPUT);
}
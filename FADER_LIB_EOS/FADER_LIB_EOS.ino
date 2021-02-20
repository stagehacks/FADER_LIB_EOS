#include <ResponsiveAnalogRead.h>
#include <NativeEthernet.h>
#include <NativeEthernetUdp.h>
#include <OSCMessage.h>
EthernetUDP Udp;


// NETWORK SETTINGS
IPAddress SELF_IP(192, 168, 1, 130);
#define SELF_PORT 58300

IPAddress DESTINATION_IP(192, 168, 1, 120);
#define DESTINATION_PORT 4710 // THIS SHOULD MATCH THE OSC UDP RX PORT IN SYSTEM SETTINGS > SHOW CONTROL > OSC

byte MAC_ADDRESS[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };


// FADER TRIM SETTINGS
#define TOP 960
#define BOT 70
int faderTrimTop[8] = {TOP, TOP, TOP, TOP, TOP, TOP, TOP, TOP}; // ADJUST THIS IF A SINGLE FADER ISN'T READING 255 AT THE TOP OF ITS TRAVEL
int faderTrimBottom[8] = {BOT, BOT, BOT, BOT, BOT, BOT, BOT, BOT}; // ADJUST THIS IF A SINGLE FADER ISN'T READING 0 AT THE BOTTOM OF ITS TRAVEL

#define TOUCH_THRESHOLD 30




static byte MOTOR_PINS_A[8] = {0, 2, 4, 6, 8, 10, 24, 28};
static byte MOTOR_PINS_B[8] = {1, 3, 5, 7, 9, 12, 25, 29};
ResponsiveAnalogRead faders[8] = {
  ResponsiveAnalogRead(A9, true),
  ResponsiveAnalogRead(A8, true),
  ResponsiveAnalogRead(A7, true),
  ResponsiveAnalogRead(A6, true),
  ResponsiveAnalogRead(A5, true),
  ResponsiveAnalogRead(A4, true),
  ResponsiveAnalogRead(A3, true),
  ResponsiveAnalogRead(A2, true)
};

void setup() {
  Serial.begin(9600);

  for (byte i = 0; i < 8; i++) {
    pinMode(MOTOR_PINS_A[i], OUTPUT);
    pinMode(MOTOR_PINS_B[i], OUTPUT);
    digitalWrite(MOTOR_PINS_A[i], LOW);
    digitalWrite(MOTOR_PINS_B[i], LOW);
    analogWriteFrequency(MOTOR_PINS_A[i], 18000);
    analogWriteFrequency(MOTOR_PINS_B[i], 18000);
    faders[i].setActivityThreshold(TOUCH_THRESHOLD);
  }
  
  Ethernet.begin(MAC_ADDRESS, SELF_IP);
  if (Ethernet.linkStatus() == LinkON) {
    Udp.begin(SELF_PORT);
  } else {
    Serial.println("Ethernet is not connected.");
  }

}

void loop() {

  for (byte i = 0; i < 8; i++) {
    faders[i].update();

    if (faders[i].hasChanged()) {
      char msg[] = "/eos/sub/x";
      msg[9] = i+49;
      
      OSCMessage outMsg(msg);
      outMsg.add(getFaderValue(i)/255.0);
      Udp.beginPacket(DESTINATION_IP, DESTINATION_PORT);
      outMsg.send(Udp);
      Udp.endPacket();
    }
  }

  delay(10);
}
int getFaderValue(byte fader) {
  return max(0, min(255, map(faders[fader].getValue(), faderTrimBottom[fader], faderTrimTop[fader], 0, 255)));
}

// FADER TRIM SETTINGS
#define TOP 960
#define BOT 70
int faderTrimTop[8] = {TOP, TOP, TOP, TOP, TOP, TOP, TOP, TOP}; // ADJUST THIS IF A SINGLE FADER ISN'T READING 255 AT THE TOP OF ITS TRAVEL
int faderTrimBottom[8] = {BOT, BOT, BOT, BOT, BOT, BOT, BOT, BOT}; // ADJUST THIS IF A SINGLE FADER ISN'T READING 0 AT THE BOTTOM OF ITS TRAVEL

// MOTOR SETTINGS
#define MOTOR_MAX_SPEED 210

// Default MIN_SPEED for Main Board version 1.0-1.2 = 170
// Default MIN_SPEED for Main Board version 1.3 = 190
// Default MIN_SPEED for Main Board version 1.4 = 145
#define MOTOR_MIN_SPEED 145

// Default MOTOR_FREQUENCY for 1.0-1.3 = 18000
// Default MOTOR_FREQUENCY for 1.4+ = 256
#define MOTOR_FREQUENCY 256

#define TOUCH_THRESHOLD 20

// ETHERNET SETTINGS
byte MAC_ADDRESS[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
int EOS_ADDRESS[] = {192, 168, 1, 120};
int OSC_UDP_TX_IP_Address[] = {192, 168, 1, 130};
int OSC_UDP_RX_Port = 8080;
int OSC_UDP_TX_Port = 8081;

#define FADER_COUNT 4
#define DEBUG false

#include <NativeEthernetUdp.h>
#include <OSCMessage.h>

#define HEARTBEAT_INTERVAL 10


EthernetUDP Udp;
uint8_t packetBuffer[UDP_TX_PACKET_MAX_SIZE];
int packetSize = 0;
IPAddress DESTINATION_IP(EOS_ADDRESS[0], EOS_ADDRESS[1], EOS_ADDRESS[2], EOS_ADDRESS[3]);
elapsedMillis sinceHeartbeat = -1000;


void loop() {
  if (getEthernetStatus() != 0) {

    packetSize = Udp.parsePacket();
    OSCMessage oscMsg;
    if (packetSize) {
      for (int j = 0; j < packetSize; j += UDP_TX_PACKET_MAX_SIZE - 1) {
        Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE - 1);
        oscMsg.fill(packetBuffer, UDP_TX_PACKET_MAX_SIZE - 1);
      }
      onOSCMessage(oscMsg);
    }

    if (sinceHeartbeat > HEARTBEAT_INTERVAL*1000) {
      sinceHeartbeat = 0;
      
      String addrStr = "/eos/fader/1/config/"+FADER_COUNT;
      char addr[21];
      addrStr.toCharArray(addr, 21);
      
      OSCMessage msg(addr);
    
      Udp.beginPacket(DESTINATION_IP, OSC_UDP_RX_Port);
      msg.send(Udp);
      Udp.endPacket();


    }
  }
  faderLoop();

}

void setup() {
  Serial.begin(9600);
  delay(1500);
  faderSetup();

}

void ethernetSetup(){
  Udp.begin(OSC_UDP_TX_Port);
  
}

void faderHasMoved(byte i) {

  if (getEthernetStatus() != 0) {
    char addr[] = "/eos/fader/1/x";
    addr[13] = i + 49;
    OSCMessage msg(addr);
    msg.add((float) getFaderValue(i)/511.0);

    Udp.beginPacket(DESTINATION_IP, 8080);
    msg.send(Udp);
    Udp.endPacket();
  }
}


void onOSCMessage(OSCMessage &msg) {
   msg.dispatch("/eos/fader/1/*", OSCFaderValue);
}
void OSCFaderValue(OSCMessage &msg) {
  char f[1];
  msg.getAddress(f, 13);
  String faderIndex = String(f);
  setFaderTarget(faderIndex.toInt()-1, msg.getFloat(0)*511);
}

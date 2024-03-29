// FADER_LIB VERSION 1.4

// Pins for FADER_8 Main Board version 1.0-1.3
//static byte MOTOR_PINS_A[8] = {0, 2, 4, 6, 8, 10, 24, 28};
//static byte MOTOR_PINS_B[8] = {1, 3, 5, 7, 9, 12, 25, 29};

// Pins for FADER_8 Main Board version 1.4+ and FADER_4 version 1.3
static byte MOTOR_PINS_A[8] = {1, 2, 5, 6, 9, 10, 36, 28};
static byte MOTOR_PINS_B[8] = {0, 3, 4, 7, 8, 12, 37, 29};


#include <TeensyThreads.h> // Ensure Board Type (Tools > Board Type) is set to Teensyduino / Teensy 4.1
#include <NativeEthernet.h>
#include <ResponsiveAnalogRead.h>

#define REST 0
#define MOTOR 1
#define TOUCH 2
#define RANGE 512

elapsedMillis sinceBegin = 0;
elapsedMillis sinceMoved[8];
elapsedMillis sinceSent[8];
byte minMotorPower[8] = {MOTOR_MIN_SPEED, MOTOR_MIN_SPEED, MOTOR_MIN_SPEED, MOTOR_MIN_SPEED, MOTOR_MIN_SPEED, MOTOR_MIN_SPEED, MOTOR_MIN_SPEED, MOTOR_MIN_SPEED};
int motorFrequency = MOTOR_FREQUENCY;
int lastSentValue[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int target[8] = {0, 0, 0, 0, 0, 0, 0, 0};
byte mode[8] = {1, 1, 1, 1, 1, 1, 1, 1};
int previous[8] = {0, 0, 0, 0, 0, 0, 0, 0};

byte readPins[8] = {A9, A8, A7, A6, A5, A4, A3, A2};
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



void faderLoop(){
  for (byte i = 0; i < FADER_COUNT; i++) {
    faders[i].update();
    int distanceFromTarget = target[i] - getUnsafeFaderValue(i);

    if (faders[i].hasChanged()) {
      sinceMoved[i] = 0;
      if (mode[i] != MOTOR && previous[i] - getFaderValue(i) != 0) {
        target[i] = -1;
        mode[i] = TOUCH;
      }
    } else if (mode[i] == REST && target[i] != -1 && abs(distanceFromTarget) >= RANGE/64) {
      mode[i] = MOTOR;
    }

    if (mode[i] == TOUCH) {
      if (lastSentValue[i] == getFaderValue(i) && sinceMoved[i] > 900) {
        mode[i] = REST;
        target[i] = -1;
      } else if (sinceSent[i] > 30 && lastSentValue[i] != getFaderValue(i)) {
        sinceMoved[i] = 0;
        sinceSent[i] = 0;
        lastSentValue[i] = getFaderValue(i);
        faderHasMoved(i);
        
      }
    }

    if (mode[i] == MOTOR) {
      faders[i].disableSleep();
      byte motorSpeed =  min(MOTOR_MAX_SPEED, minMotorPower[i] + abs(distanceFromTarget / (RANGE/32)));

      if (abs(distanceFromTarget) < RANGE/64) {
        analogWrite(MOTOR_PINS_A[i], 255);
        analogWrite(MOTOR_PINS_B[i], 255);
        if (sinceMoved[i] > 10) {
          faders[i].enableSleep();
        }
        if (sinceMoved[i] > 200) {
          mode[i] = REST;
          target[i] = -1;
        }
      } else if (distanceFromTarget > 0) {
        sinceMoved[i] = 0;
        analogWrite(MOTOR_PINS_A[i], motorSpeed);
        analogWrite(MOTOR_PINS_B[i], 0);
      } else if (distanceFromTarget < 0) {
        sinceMoved[i] = 0;
        analogWrite(MOTOR_PINS_A[i], 0);
        analogWrite(MOTOR_PINS_B[i], motorSpeed);
      }

    } else {
      faders[i].enableSleep();
      analogWrite(MOTOR_PINS_A[i], 0);
      analogWrite(MOTOR_PINS_B[i], 0);
    }
    if (DEBUG) {
      if(mode[i]==MOTOR){
        Serial.print("M");
      }else if(mode[i]==TOUCH){
        Serial.print("T");
      }else if(mode[i]==REST){
        Serial.print(" ");
      }
      Serial.print("-");
      Serial.print(getFaderValue(i));
      Serial.print("\t");
    }
    previous[i] = getFaderValue(i);

  }
  if (DEBUG) {
    Serial.println("");
  }
}


byte networkThreadID = 0;
void faderSetup() {
  
  for (byte i = 0; i < FADER_COUNT; i++) {
    pinMode(MOTOR_PINS_A[i], OUTPUT);
    pinMode(MOTOR_PINS_B[i], OUTPUT);
    digitalWrite(MOTOR_PINS_A[i], LOW);
    digitalWrite(MOTOR_PINS_B[i], LOW);
    analogWriteFrequency(MOTOR_PINS_A[i], motorFrequency);
    analogWriteFrequency(MOTOR_PINS_B[i], motorFrequency);
    faders[i].setActivityThreshold(TOUCH_THRESHOLD);
  }
  
  delay(2000);

  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.print("########  FADER_");
  Serial.print(FADER_COUNT);
  Serial.println(" ########");
  Serial.println("Calibrating Faders...");

  for(byte i=0; i<FADER_COUNT; i++){
    faders[i].disableSleep();
    faders[i].update();
    int initial = getFaderValue(i);

    digitalWrite(LED_BUILTIN, !(i%2));
    
    while(abs(getFaderValue(i)-initial)<RANGE/32){
      Serial.println(getFaderValue(i));
      faders[i].update();
      minMotorPower[i]++;
      if(initial<RANGE/2){
        analogWrite(MOTOR_PINS_A[i], minMotorPower[i]);
        analogWrite(MOTOR_PINS_B[i], 0);
      }else{
        analogWrite(MOTOR_PINS_A[i], 0);
        analogWrite(MOTOR_PINS_B[i], minMotorPower[i]);
      }
      
      delay(10);
    }
    faders[i].enableSleep();
    //minMotorPower[i]+=2;
    analogWrite(MOTOR_PINS_A[i], 0);
    analogWrite(MOTOR_PINS_B[i], 0);
    Serial.print(minMotorPower[i]);
    Serial.print(",");
  }
  Serial.println("");
  
  Serial.println("Starting Network Thread...");
  networkThreadID = threads.addThread(networkInit);
}


#define ETHERNET_OFFLINE 0
#define ETHERNET_ONLINE_DHCP 1
#define ETHERNET_ONLINE_STATIC 2
byte ethernetStatus = 0;

void networkInit() {
  Serial.println("Testing Ethernet Connection...");

  IPAddress SELF_IP(OSC_UDP_TX_IP_Address[0], OSC_UDP_TX_IP_Address[1], OSC_UDP_TX_IP_Address[2], OSC_UDP_TX_IP_Address[3]);

  Ethernet.begin(MAC_ADDRESS, SELF_IP);
  ethernetStatus = ETHERNET_ONLINE_STATIC;

  Serial.print("Ethernet Status: ");
  Serial.println(Ethernet.linkStatus());

  if (Ethernet.linkStatus() == LinkON) {
    ethernetSetup();
  } else {
    Serial.println("Ethernet is not connected.");
  }

  threads.suspend(networkThreadID);

}


void setFaderTarget(byte fader, int value){
  target[fader] = value;
}

byte getEthernetStatus(){
  return ethernetStatus;
}

int getFaderValue(byte fader) {
  return max(0, min(RANGE-1, map(faders[fader].getValue(), faderTrimBottom[fader], faderTrimTop[fader], 0, RANGE-1)));
}
int getUnsafeFaderValue(byte fader){
  return map(faders[fader].getValue(), faderTrimBottom[fader], faderTrimTop[fader], 0, RANGE-1);
}

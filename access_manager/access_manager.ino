#include <Keypad.h>

#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <EthernetUdp.h>         // UDP library from: bjoern@cs.stanford.edu 12/30/2008

const boolean DEBUG = false;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xDD };

IPAddress local_ip(192, 168, 0, 201);
const unsigned int udp_port = 54545;      // local port to listen on

IPAddress server_ip(192, 168, 0, 252);

const long KEEP_ALIVE_INTERVAL =  180000;
unsigned long nextDeviceKeepAliveTime = 0;

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
char  ReplyBuffer[] = "acknowledged";       // a string to send back

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

// Keypad stuff
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
// Connect keypad ROW0 (1,2,3), ROW1 (4,5,6), ROW2 and ROW3 to these Arduino pins.
byte rowPins[ROWS] = {5, 4, 3, 2}; //connect to the row pinouts of the keypad
// Connect keypad COL0 (1,4,7,*), COL1 (2,5,8,0) and COL2 to these Arduino pins.
byte colPins[COLS] = {8, 7, 6};        //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

const int TIME_OUT_DURATION = 3000;
const int LOCK_OPEN_DURATION = 250;
unsigned long timeout = 0;
String keyPresses = "";

const int PASSWORDS = 6;
String passwords[PASSWORDS] = { "0909", "0504", "0805", "2907", "1234", "4321" };
String who[PASSWORDS]       = { "Renen", "Ros", "Sofie", "Finn", "Josephine", "ADT" };

const int GATE_LOCK_PIN = 9;

void setup() {
  Serial.begin(9600);
  Serial.println("Starting");
  
  pinMode(GATE_LOCK_PIN, OUTPUT);
  
  Ethernet.begin(mac,local_ip);
  Udp.begin(udp_port);
  
}

void loop() {
  
  unsigned long currentTime = millis();
  
  if (currentTime >= nextDeviceKeepAliveTime) {
      sendKeepAlive(currentTime);
  }
  
  if ((keyPresses.length()>0) && (currentTime>timeout)) {
    keyPresses = "";
  }
  
  char key = keypad.getKey();
  if (key) {
    //Serial.println(key);
    timeout = currentTime + TIME_OUT_DURATION;
    if (key=='#') {
      if (keyPresses.length()>0) {
        String accessor = checkPassword(keyPresses);
        if (accessor.length()>0) {
          openGateLock();
          udp("front_gate", accessor);
        } else {
          udp("front_gate_fail", keyPresses);
        }
        keyPresses = "";
      }
    } else {
      keyPresses = keyPresses + key;
      if (keyPresses.length()>20) {
        udp("front_gate_fail", keyPresses);
        keyPresses = "";
      }
    }
  }

}

String checkPassword(String keyPresses) {
  String accessor = "";
  int i;
  for(i=0; i<PASSWORDS; i=i+1) {
    if (passwords[i]==keyPresses) {
      accessor = who[i];
      break;
    }
  }
  return accessor;
}

void openGateLock() {
  Serial.println("open");
  digitalWrite(GATE_LOCK_PIN, HIGH);
  delay(LOCK_OPEN_DURATION);
  Serial.println("close");
  digitalWrite(GATE_LOCK_PIN, LOW);
}

void sendKeepAlive(unsigned long currentTime) {
  udp("front_gate_alive", "1");
  nextDeviceKeepAliveTime = currentTime + KEEP_ALIVE_INTERVAL;
}

void udp(String sensor, String message) {
  String payload = sensor + " " + message;
  char p[50];
  payload.toCharArray(p, 50);
  if (DEBUG==false) {
    Udp.beginPacket(server_ip, udp_port);
    Udp.write(p);
    Udp.endPacket();
  } else {
    Serial.println(p);
  }
}




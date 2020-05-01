#include <stdlib.h>
#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <EthernetUdp.h>         // UDP library from: bjoern@cs.stanford.edu 12/30/2008

const boolean DEBUG = false;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xDF };

IPAddress local_ip(192, 168, 0, 204);
IPAddress server_ip(192, 168, 0, 252);

const long KEEP_ALIVE_INTERVAL =  180000;
unsigned long nextDeviceKeepAliveTime = 0;

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;
const unsigned int udp_port = 54545;      // local port to send to

#define GAUGE_PIN 2
unsigned long debounceTime = 0;
const unsigned int DEBOUNCE_DURATION = 300;

void setup() {
  Serial.begin(9600);
  pinMode(GAUGE_PIN, INPUT_PULLUP);        //set pin 2 as an output, and make the device use its internal pull up resistor.
  Ethernet.begin(mac,local_ip);
  Udp.begin(udp_port);
  debounceTime = millis() + DEBOUNCE_DURATION;
  Serial.println("Initialised...");
}

void loop() {
  
  unsigned long currentTime = millis();
  
  if ((currentTime>debounceTime)&&(digitalRead(GAUGE_PIN)==HIGH)){
    debounceTime = currentTime + DEBOUNCE_DURATION;
    sendClick();
  }
  
  if (currentTime >= nextDeviceKeepAliveTime) {
    sendKeepAlive(currentTime);
    nextDeviceKeepAliveTime = currentTime + KEEP_ALIVE_INTERVAL;
  }
  
}

void sendClick() {
  udp("rain_gauge", "1");
}

void sendKeepAlive(unsigned long currentTime) {
  udp("rain_gauge_alive", "1");
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


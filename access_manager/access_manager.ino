#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <EthernetUdp.h>         // UDP library from: bjoern@cs.stanford.edu 12/30/2008

const boolean DEBUG = true;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xDD };

IPAddress local_ip(192, 168, 0, 201);
const unsigned int udp_port = 54545;      // local port to listen on

IPAddress server_ip(192, 168, 0, 252);

const long KEEP_ALIVE_INTERVAL =  5000;
unsigned long nextDeviceKeepAliveTime = 0;

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
char  ReplyBuffer[] = "acknowledged";       // a string to send back

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

void setup() {
  Serial.begin(9600);
  Serial.println("Starting");

  Ethernet.begin(mac,local_ip);
  Udp.begin(udp_port);
  
}

void loop() {
  unsigned long currentTime = millis();
  
  if (currentTime >= nextDeviceKeepAliveTime) {
      sendKeepAlive(currentTime);
  }

}

void sendKeepAlive(unsigned long currentTime) {
  udp("access_alive", "1");
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




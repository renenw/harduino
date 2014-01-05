#include <stdlib.h>
#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <EthernetUdp.h>         // UDP library from: bjoern@cs.stanford.edu 12/30/2008

const boolean DEBUG = false;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEF };

IPAddress local_ip(192, 168, 0, 202);
const unsigned int udp_port = 54545;      // local port to listen on

IPAddress server_ip(192, 168, 0, 252);

const long KEEP_ALIVE_INTERVAL =  180000;
unsigned long nextDeviceKeepAliveTime = 0;
const long TEMPERATURE_UPDATE_INTERVAL = 300000;
unsigned long nextTemperatureUpdate   = 0;

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
char  ReplyBuffer[] = "acknowledged";       // a string to send back

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

void setup() {
  Serial.begin(9600);
  pinMode(2, OUTPUT);        //set pin 2 as an output
  Ethernet.begin(mac,local_ip);
  Udp.begin(udp_port);
  Serial.println("Initialised...");
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime >= nextDeviceKeepAliveTime) {
      sendKeepAlive(currentTime);
  }
  if (currentTime >= nextTemperatureUpdate) {
      sendPoolTemperature(currentTime);
  }
}

float read_temp(void) {   //the read temperature function
  float v_out;             //voltage output from temp sensor 
  float temp;              //the final temperature is stored here
  digitalWrite(A0, LOW);   //set pull-up on analog pin
  digitalWrite(2, HIGH);   //set pin 2 high, this will turn on temp sensor
  delay(2);                //wait 2 ms for temp to stabilize
  v_out = analogRead(0);   //read the input pin
  digitalWrite(2, LOW);    //set pin 2 low, this will turn off temp sensor
  v_out*=.0048;            //convert ADC points to volts (we are using .0048 because this device is running at 5 volts)
  v_out*=1000;             //convert volts to millivolts
  temp = 0.0512 * v_out -20.5128; //the equation from millivolts to temperature
  return temp;             //send back the temp
}

void sendPoolTemperature(unsigned long currentTime) {
  float temp;
  temp = read_temp();       //call the function “read_temp” and return the temperature in C°
  udp_float("pool_temperature", temp);
  nextTemperatureUpdate = currentTime + TEMPERATURE_UPDATE_INTERVAL;
}

void sendKeepAlive(unsigned long currentTime) {
  udp("pool_temperature_alive", "1");
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

void udp_float(String sensor, float f) {
  String payload = sensor + " ";
  char p[50];
  payload.toCharArray(p, 50); // this call wants payload to be local
  char c[50];  // overkill
  dtostrf(f, 6, 2, c);
  if (DEBUG==false) {
    Udp.beginPacket(server_ip, udp_port);
    Udp.write(p);
    Udp.write(' ');
    Udp.write(c);
    Udp.endPacket();
  } else {
    Serial.print(p);
    Serial.print(' ');
    Serial.println(c);
  }
}

/*
  UDPSendReceive.pde:
 This sketch receives UDP message strings, prints them to the serial port
 and sends an "acknowledge" string back to the sender
 
 A Processing sketch is included at the end of file that can be used to send 
 and received messages for testing with a computer.
 
*/


#include <SPI.h>         // needed for Arduino versions later than 0018
#include <Ethernet.h>
#include <EthernetUdp.h>         // UDP library from: bjoern@cs.stanford.edu 12/30/2008


// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

IPAddress local_ip(192, 168, 0, 200);
const unsigned int udp_port = 54545;      // local port to listen on

IPAddress server_ip(50, 19, 129, 102);

unsigned long nextDeviceKeepAliveTime = 0;

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
char  ReplyBuffer[] = "acknowledged";       // a string to send back

// alarm state
#define ALARM_PIN 2 
const long ALARM_UPDATE_INTERVAL = 3600000;
volatile boolean alarmChanged = false;
volatile unsigned long nextServerAlarmUpdateTime = 0;

// alarm activation
#define ALARM_ACTIVATION_PIN 3
const byte ALARM_ACTIVATION_THRESHOLD = 3;
volatile int alarmActivations = 0;
volatile unsigned long timeAtWhichAlarmBackToNormal = 0;
const long ALARM_ACTIVATION_TIMEOUT = 10000;
boolean alarmActivated = false;

// pool depth
#define POOL_DEPTH_SERIES_RESISTOR 470    
#define POOL_DEPTH_SENSOR_PIN A0
#define POOL_DEPTH_NUMBER_SAMPLES 5
unsigned long nextPoolDepthCheck= 0;
const long POOL_DEPTH_CHECK_INTERVAL = 5000;
int pool_depth_samples[POOL_DEPTH_NUMBER_SAMPLES];

// pool soldenoid
#define POOL_SOLENOID_PIN 5
boolean poolSolenoidState = false;
unsigned long nextPoolSolenoidSwitch = 0;

// OPTO
#define OPTO_PIN 6

#define SIMPLE_PIN 7

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

void setup() {
  Serial.begin(9600);
  Serial.println("Starting");

  Ethernet.begin(mac,local_ip);
  Udp.begin(udp_port);
  
  // alarm monitoring stuff
  pinMode(ALARM_PIN, INPUT);
  digitalWrite(ALARM_PIN, HIGH);       // turn on internal pullup resistors
  attachInterrupt(0, catch_alarm_change, CHANGE);
  
  // alarm activations
  pinMode(ALARM_ACTIVATION_PIN, INPUT);
  digitalWrite(ALARM_ACTIVATION_PIN, HIGH);       // turn on internal pullup resistors
  attachInterrupt(1, catch_alarm_activation, CHANGE);
  
  // pool depth
  analogReference(EXTERNAL);
  
  // pool solenoid
  pinMode(POOL_SOLENOID_PIN, OUTPUT);
  digitalWrite(POOL_SOLENOID_PIN, LOW);

  // opto
  pinMode(OPTO_PIN, INPUT);
  digitalWrite(OPTO_PIN, HIGH);       // turn on internal pullup resistors
  
  // simple
  pinMode(SIMPLE_PIN, INPUT);
  digitalWrite(SIMPLE_PIN, LOW);
  
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime >= nextServerAlarmUpdateTime) {
    reportAlarmState(currentTime);
  }
  if (alarmActivations > 0) {
    //checkForAlarmActivation(currentTime);
  }
  if ((alarmActivated==true) && (currentTime >= timeAtWhichAlarmBackToNormal)) {
    //alarmActivationOver();
  }
  if (currentTime >= nextPoolSolenoidSwitch) {
//    switchPoolSolenoid(currentTime);
  }
  if (currentTime >= nextPoolDepthCheck) {
//    poolDepthCheck(currentTime);
  }
  
  //check_simple(); alarm circuits
  
  if (currentTime >= nextDeviceKeepAliveTime) {
      sendKeepAlive(currentTime);
  }

}

void sendKeepAlive(unsigned long currentTime) {
  udp("a0", "1");
  nextDeviceKeepAliveTime = currentTime + 30000;
}

void udp(String sensor, String message) {
  String payload = sensor + " " + message;
  char p[50];
  payload.toCharArray(p, 50);
  Udp.beginPacket(server_ip, udp_port);
  Udp.write(p);
  Udp.endPacket();
}

void check_simple() {
  int simpleState = digitalRead(SIMPLE_PIN);
  Serial.println( simpleState );

}

void check_opto() {
  int optoState = digitalRead(OPTO_PIN);
  Serial.println( optoState );
}

void switchPoolSolenoid(unsigned long currentTime) {
  poolSolenoidState = !poolSolenoidState;
  Serial.print("Solenoid: ");
  Serial.println(poolSolenoidState);
  if (poolSolenoidState) {
    digitalWrite(POOL_SOLENOID_PIN, HIGH);
  } else {
    digitalWrite(POOL_SOLENOID_PIN, LOW);
  }
  nextPoolSolenoidSwitch = currentTime + 5000;
}

void poolDepthCheck(unsigned long currentTime) {

  uint8_t i;
  float average;
 
  // take N samples in a row, with a slight delay
  for (i=0; i< POOL_DEPTH_NUMBER_SAMPLES; i++) {
   pool_depth_samples[i] = analogRead(POOL_DEPTH_SERIES_RESISTOR);
   //Serial.print("Reading ");
   //Serial.print(i);
   //Serial.print(": ");
   //Serial.println(pool_depth_samples[i]);
   delay(10);
  }
 
  // average all the samples out
  average = 0;
  for (i=0; i< POOL_DEPTH_NUMBER_SAMPLES; i++) {
     average += pool_depth_samples[i];
  }
  average /= POOL_DEPTH_NUMBER_SAMPLES;
 
  //Serial.print("Average analog reading "); 
  //Serial.println(average);
  // convert the value to resistance
  average = 1023 / average - 1;
  average = POOL_DEPTH_SERIES_RESISTOR / average;
 
  Serial.print("Depth resistance "); 
  Serial.println(average);
 
  nextPoolDepthCheck = currentTime + POOL_DEPTH_CHECK_INTERVAL;
}

void catch_alarm_change() {
  alarmChanged = true;
  nextServerAlarmUpdateTime = millis();
}

void catch_alarm_activation() {
  alarmActivations = alarmActivations + 1;
}

void reportAlarmState(unsigned long currentTime) {
  
  // if this was triggered by an interrupt, debounce (pause for 50ms, and then check the state again)
  if (alarmChanged==true) {
    delay(50);
    alarmChanged = false;
  }
  
  int alarmState = digitalRead(ALARM_PIN);
  String state;
  if (alarmState==1) {
    state = "0";
  } else {
    state = "1";
  }
  udp("aa", state);
  
  //Serial.print(currentTime);
  //Serial.print(": ");
  //Serial.print(alarmState);
  //Serial.print(" and the alarm is going off: ");
  //Serial.println(alarmActivated);
  nextServerAlarmUpdateTime = currentTime + ALARM_UPDATE_INTERVAL;
  
}

void checkForAlarmActivation(unsigned long currentTime) {
  delay(500);
  if (alarmActivations>ALARM_ACTIVATION_THRESHOLD) {
    if (alarmActivated==false) {
      Serial.println("Alarm going off");
    }
    alarmActivated = true;
    timeAtWhichAlarmBackToNormal = currentTime + ALARM_ACTIVATION_TIMEOUT;
  }
  alarmActivations = 0;
}

void alarmActivationOver() {
  alarmActivated = false;
  Serial.println("Alarm no longer going off");
}

/*
  Processing sketch to run with this example
 =====================================================
 
 // Processing UDP example to send and receive string data from Arduino 
 // press any key to send the "Hello Arduino" message
 
 
 import hypermedia.net.*;
 
 UDP udp;  // define the UDP object
 
 
 void setup() {
 udp = new UDP( this, 6000 );  // create a new datagram connection on port 6000
 //udp.log( true ); 		// <-- printout the connection activity
 udp.listen( true );           // and wait for incoming message  
 }
 
 void draw()
 {
 }
 
 void keyPressed() {
 String ip       = "192.168.1.177";	// the remote IP address
 int port        = 8888;		// the destination port
 
 udp.send("Hello World", ip, port );   // the message to send
 
 }
 
 void receive( byte[] data ) { 			// <-- default handler
 //void receive( byte[] data, String ip, int port ) {	// <-- extended handler
 
 for(int i=0; i < data.length; i++) 
 print(char(data[i]));  
 println();   
 }
 */



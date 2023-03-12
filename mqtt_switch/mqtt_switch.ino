#include <EthernetClient.h>
#include <PubSubClient.h>
#include <SPI.h>         // needed for Arduino versions later than 0018

/*
https://learn.sparkfun.com/tutorials/introduction-to-mqtt/all
http://www.steves-internet-guide.com/using-arduino-pubsub-mqtt-client/
http://forum.arduino.cc/index.php?topic=102978.0
http://www.sainsmart.com/8-channel-dc-5v-relay-module-for-arduino-pic-arm-dsp-avr-msp430-ttl-logic.html#customer-reviews (Product review by Karl, 11/17/14)
*/

const boolean DEBUG = false;

const int SWITCHES = 16;
const int switchPinMapping[SWITCHES] = {22, 24, 26, 28, 30, 32, 34, 36,  23, 25, 27, 29, 31, 33, 35, 37};
const int LED = 13;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xDE };
IPAddress localIp(192, 168, 0, 203);

EthernetClient ethClient;
PubSubClient mqttClient(ethClient);

// const long KEEP_ALIVE_INTERVAL =  180000;
const long KEEP_ALIVE_INTERVAL =  30000;
unsigned long nextDeviceKeepAliveTime = 0;

// Switch specific actions
boolean selfTest = false;
unsigned long deEnergiseAt[SWITCHES];
int energise = -1;
unsigned long energiseDuration = -1;
unsigned long DEFAULT_ENERGISE_DURATION = 20*60;

// keep board light blinking
int BLINK_DURATION = 2500;
boolean blinkOn = false;
unsigned long blinkChangeAt = 0;

void setup() {                
  Serial.begin(9600);
  Serial.println("starting");
  initialisePins();
  initialiseNetwork();
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("initialisation complete");
}

void loop() {
  unsigned long currentTime = millis();
  if (!mqttClient.connected()) { connect(); }
  if (selfTest)       { runSelfTests(); }
  if (energise!=-1)   { handleEnergiseInstruction(); }
  deEnergiseRelaysThatHaveTimedOut();
  if (currentTime >= blinkChangeAt) { blinkLight(currentTime); }
  if (currentTime >= nextDeviceKeepAliveTime) { sendKeepAlive(currentTime); }
}


void deEnergiseRelaysThatHaveTimedOut() {
  for (int n = 0; n < SWITCHES; n++) {
    if ( deEnergiseAt[n] > 0 && deEnergiseAt[n] < millis() ) {
      Serial.print("Timeout: ");
      Serial.println(n);
      deEnergiseRelay(n);
    }    
  }  
}

void runSelfTests() {
  selfTest = false;
  runSelfTestPins();
}

void handleEnergiseInstruction() {
  int n = energise;
  energise = -1;
  deEnergiseAt[n] = millis() + energiseDuration * 1000;
  energiseDuration = -1;
  energiseRelay(n);
}

int pin(int n) {
  return switchPinMapping[n];
}

void energiseRelay(int n) {
  digitalWrite(pin(n), LOW);
  mqtt_send("switch_" + String(n, DEC), "on");
}

void deEnergiseRelay(int n) {
  digitalWrite(pin(n), HIGH);
  deEnergiseAt[n] = 0;
  mqtt_send("switch_" + String(n, DEC), "off");
}

void initialisePins() { 
  for (int n = 0; n < SWITCHES; n++) {
    int pin = switchPinMapping[n];
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    deEnergiseAt[n] = 0;
  }
  pinMode(LED, OUTPUT);
  Serial.println("Pin initialisation complete");
}

void callback(char* topic, byte* payload, unsigned int length) {
  
  String instruction;
  int n = -1;
  int energiseFor = DEFAULT_ENERGISE_DURATION;

  for (int i = 0; i < length; i++) {
    instruction += (char)payload[i];
  }
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(instruction);

  if (strcmp(topic,"switcher")==0){
    if (instruction=="test") {
      selfTest = true;
    }
  }

  if (strcmp(topic,"switcher_0")==0) { n = 0; }
  if (strcmp(topic,"switcher_1")==0) { n = 0; }
  if (strcmp(topic,"switcher_2")==0) { n = 0; }
  if (strcmp(topic,"switcher_3")==0) { n = 0; }
  if (strcmp(topic,"switcher_4")==0) { n = 0; }
  if (strcmp(topic,"switcher_5")==0) { n = 0; }
  if (strcmp(topic,"switcher_6")==0) { n = 0; }
  if (strcmp(topic,"switcher_7")==0) { n = 0; }
  if (strcmp(topic,"switcher_8")==0) { n = 0; }
  if (strcmp(topic,"switcher_9")==0) { n = 0; }
  if (strcmp(topic,"switcher_10")==0) { n = 0; }
  if (strcmp(topic,"switcher_11")==0) { n = 0; }
  if (strcmp(topic,"switcher_12")==0) { n = 0; }
  if (strcmp(topic,"switcher_13")==0) { n = 0; }
  if (strcmp(topic,"switcher_14")==0) { n = 0; }
  if (strcmp(topic,"switcher_15")==0) { n = 0; }

  if (n>=0) {
    if (instruction.length()!=0) {
      if (instruction=="on") {
        // energiseFor = DEFAULT_ENERGISE_DURATION;
      } else if (instruction=="off") {
        energiseFor = 0;
      } else {
        energiseFor = instruction.toInt();
      }
    }
    energise = n;
    energiseDuration = energiseFor;
  }
  
}

void initialiseNetwork() {
  Ethernet.begin(mac,localIp);
  Serial.println("Device has ip address ");
  Serial.println(Ethernet.localIP());
  Serial.println("Ethernet initialisation complete");
}

void initialiseMqtt() {
  mqttClient.setServer("192.168.0.245", 1883);
  mqttClient.setCallback(callback);
  Serial.println("Basic MQTT configuration complete.");
  connect();
}


void connect() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if(mqttClient.connect("switcher")) {
      mqttClient.subscribe("switcher/switch");
      mqttClient.subscribe("switcher/0");
      mqttClient.subscribe("switcher/1");
      mqttClient.subscribe("switcher/2");
      mqttClient.subscribe("switcher/3");
      mqttClient.subscribe("switcher/4");
      mqttClient.subscribe("switcher/5");
      mqttClient.subscribe("switcher/6");
      mqttClient.subscribe("switcher/7");
      mqttClient.subscribe("switcher/8");
      mqttClient.subscribe("switcher/9");
      mqttClient.subscribe("switcher/10");
      mqttClient.subscribe("switcher/11");
      mqttClient.subscribe("switcher/12");
      mqttClient.subscribe("switcher/13");
      mqttClient.subscribe("switcher/14");
      mqttClient.subscribe("switcher/15");
    } else {
      Serial.println("Failed to connect. Will retry in 5 seconds.");
      delay(5000);
    }
  }
}


void runSelfTestPins() {
  for (int n = 0; n < SWITCHES; n++) {
    energiseRelay(n);
    delay(1000);
    deEnergiseRelay(n);
    delay(100);
  }
}

void mqtt_send(String sensor, String message) {
  String topic = "switcher/" + sensor + "/state";
  mqttClient.publish(topic.c_str(), message.c_str());
}


void blinkLight(unsigned long currentTime) {
  if (blinkOn) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
  blinkOn = !blinkOn;
  blinkChangeAt = currentTime + BLINK_DURATION;
}

void sendKeepAlive(unsigned long currentTime) {
  mqtt_send("switch", String(currentTime, DEC));
  nextDeviceKeepAliveTime = currentTime + KEEP_ALIVE_INTERVAL;
}

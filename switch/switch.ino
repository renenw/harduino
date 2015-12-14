#include <Ethernet2.h>
#include <EthernetUdp2.h>
#include <SPI.h>         // needed for Arduino versions later than 0018

/*
http://forum.arduino.cc/index.php?topic=102978.0
http://www.sainsmart.com/8-channel-dc-5v-relay-module-for-arduino-pic-arm-dsp-avr-msp430-ttl-logic.html#customer-reviews (Product review by Karl, 11/17/14)
*/

const boolean DEBUG = false;
const String DEVICE_NAME = "switcher";

const int SWITCHES = 16;
const int switchPinMapping[SWITCHES] = {22, 24, 26, 28, 30, 32, 34, 36,  23, 25, 27, 29, 31, 33, 35, 37};
const int LED = 13;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xDE };
IPAddress localIp(192, 168, 0, 203);
// IPAddress localIp(10, 0, 0, 2);

const unsigned int udp_port = 54545;
IPAddress server_ip(192, 168, 0, 252);

const long KEEP_ALIVE_INTERVAL =  180000;
// const long KEEP_ALIVE_INTERVAL =  5000;
unsigned long nextDeviceKeepAliveTime = 0;

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
char ReplyBuffer[] = "acknowledged";       // a string to send back

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;
EthernetServer server(80);

// Keep a little history
const int LOG_ITEMS = 50;
int logPointer = 0;
String logMessages[LOG_ITEMS];
unsigned long logTimes[LOG_ITEMS];
String currentLine = "";

// Switch specific actions
boolean selfTest = false;
unsigned long deEnergiseAt[SWITCHES];
int energise = -1;
int deEnergise = -1;
unsigned long energiseDuration = -1;
unsigned long DEFAULT_ENERGISE_DURATION = 20*60;

void setup() {                
  Serial.begin(9600);
  logLn("starting");
  initialisePins();
  initialiseNetwork();
  logLn("initialisation complete");
}

void loop() {
  unsigned long currentTime = millis();
  if (selfTest)       { runSelfTests(); }
  if (energise!=-1)   { handleEnergiseInstruction(); }
  if (deEnergise!=-1) { handleDeEnergiseInstruction(); }
  deEnergiseRelaysThatHaveTimedOut();
  dealWithWebRequests();
  if (currentTime >= nextDeviceKeepAliveTime) { sendKeepAlive(currentTime); }
}

String handle_web_request(EthernetClient client, String request) {
  String outcome = "";
  int spaceTerminator = request.lastIndexOf(' ');
  if (spaceTerminator!=-1) {
    request = request.substring(0, spaceTerminator);
  }
  if ( request.startsWith("PUT /on") || (DEBUG && (request.startsWith("GET /on"))) ) {
    outcome = web_put_on(client, request);
  } else if ( request.startsWith("PUT /off") || (DEBUG && (request.startsWith("GET /off"))) ) {
    outcome = web_put_off(client, request);
  } else if ( DEBUG && (request.startsWith("GET /test")) ) {
    outcome = web_handle_test_request(client);
  } else if (request.startsWith("GET /")) {
    outcome = web_get_index(client);
  }
  if (outcome=="") {
    outcome = "404 Not Found";
    write_response_headers(client, outcome, "*/*", 0);
  }
  return outcome;
}

void deEnergiseRelaysThatHaveTimedOut() {
  for (int n = 0; n < SWITCHES; n++) {
    if ( deEnergiseAt[n] > 0 && deEnergiseAt[n] < millis() ) {
      logC("Timeout: ");
      logLn(n);
      deEnergiseRelay(n);
    }    
  }  
}

void runSelfTests() {
  selfTest = false;
  runSelfTestPins();
  runSelfTestNetwork(); 
}

void handleEnergiseInstruction() {
  int n = energise;
  energise = -1;
  deEnergiseAt[n] = millis() + energiseDuration * 1000;
  energiseDuration = -1;
  energiseRelay(n);
}

void handleDeEnergiseInstruction() {
  int n = deEnergise;
  deEnergise = -1;
  deEnergiseRelay(n);
}

int pin(int n) {
  return switchPinMapping[n];
}

void energiseRelay(int n) {
  digitalWrite(pin(n), LOW);
  udp("switch_" + String(n, DEC), "1");
}

void deEnergiseRelay(int n) {
  digitalWrite(pin(n), HIGH);
  deEnergiseAt[n] = 0;
  udp("switch_" + String(n, DEC), "0");
}

void initialisePins() { 
  for (int n = 0; n < SWITCHES; n++) {
    int pin = switchPinMapping[n];
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
    deEnergiseAt[n] = 0;
  }
  pinMode(LED, OUTPUT);
  logLn("pin initialisation complete");
}

void initialiseNetwork() {
  Ethernet.begin(mac,localIp);
  logC ("device has ip address ");
  logLn(Ethernet.localIP());
  logLn("ethernet initialisation complete");
  Udp.begin(udp_port);
  logLn("udp initialisation complete");
  server.begin();
  logLn("http initialisation complete");
}

void runSelfTestNetwork() {
  udp("switch", "hello!");
}

void runSelfTestPins() {
  for (int n = 0; n < SWITCHES; n++) {
    energiseRelay(n);
    delay(500);
    deEnergiseRelay(n);
    delay(100);
  }
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
    Serial.print("UDP Debug Message: ");
    Serial.println(p);
  }
  logC("UDP: ");
  logLn(p);
}

void logWebOutcome(String request, String outcome, unsigned long startTime, unsigned long endTime) {
  logC("Web: ");
  logC(startTime);
  logC(" \"");
  logC(request);
  logC("\" ");
  logC(outcome.substring(0, 3));
  logC(" ");
  logC(endTime-startTime);
  logLn("ms");
}

void dealWithWebRequests() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    boolean firstLine = true;
    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (DEBUG) { Serial.write(c); }  // uncomment to echo web request to serial
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          unsigned long startTime = millis();
          String outcome = handle_web_request(client, request);
          unsigned long endTime = millis();
          logWebOutcome(request, outcome, startTime, endTime);
          break;
        }
        if (c == '\n') {
          currentLineIsBlank = true;
          firstLine = false;
        }
        else if (c != '\r') {
          currentLineIsBlank = false;
          if (firstLine) { request = request + c; }
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }
}

void write_response_headers(EthernetClient client, String httpStatus, String contentType, int refresh) {
  client.print("HTTP/1.1 ");
  client.println(httpStatus);
  client.print("Content-Type: ");
  client.println(contentType);
  client.println("Connection: close");  // the connection will be closed after completion of the response
  if (refresh>0) {
    client.print("Refresh: ");
    client.println(refresh);
  }
  client.println(); 
}

String web_put_on(EthernetClient client, String request) {
  
  String httpResponseCode = "400 Bad Request";
  long switchNumber = getSwitchNumberFromQueryString(request);
  long energiseFor  = getDurationFromQueryString(request);
  if ((switchNumber>-1) && (energiseFor>0)) {
    energise = switchNumber;
    energiseDuration = energiseFor;
    httpResponseCode = "202 Accepted";
  }
  
  if (DEBUG) {
    write_response_headers(client, httpResponseCode, "text/plain", 0);
    client.print("Switch On: ");
    client.println(switchNumber);
    client.print("For: ");
    client.print(energiseFor);
    client.println("s");
  } else {
    write_response_headers(client, httpResponseCode, "*/*", 0);
  }
  
  return httpResponseCode;
}

String web_put_off(EthernetClient client, String request) {
  
  String httpResponseCode = "400 Bad Request";
  long switchNumber = getSwitchNumberFromQueryString(request);
  
  if (switchNumber>-1) {
    deEnergise = switchNumber;
    httpResponseCode = "202 Accepted";
  }
  
  if (DEBUG) {
    write_response_headers(client, httpResponseCode, "text/plain", 0);
    client.print("Switch Off: ");
    client.println(switchNumber);
  } else {
    write_response_headers(client, httpResponseCode, "*/*", 0);
  }
  
  return httpResponseCode;
}

String web_handle_test_request(EthernetClient client) {
  write_response_headers(client, "202 Accepted", "text/plain", 0);
  selfTest = true;
  client.println("Self test initiated.");
  return "202 Accepted";
}

String extractValueFromQueryString(String request, String key) {
  String value = "";
  int qsStart = request.indexOf(key);
  if (qsStart!=-1) {
    qsStart = qsStart + key.length() + 1;
    int qsEnd = request.indexOf('&', qsStart);
    if (qsEnd==-1) {
      value = request.substring(qsStart);
    } else {
      value = request.substring(qsStart, qsEnd);
    }
  }
  return value;
}

String web_get_index(EthernetClient client) {
  write_response_headers(client, "200 OK", "text/html", 0);
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.print("<h2>At: ");
  client.print(millis());
  client.print("</h2>");
  
  client.println("<h1>Switches</h1>");
  for (int n = 0; n < SWITCHES; n++) {
    String state = "unknown";
    if (digitalRead(switchPinMapping[n])==HIGH) {
      state = "Off";
    } else {
      state = "On";
    }
    client.print("Switch: ");
    client.print(n);
    client.print(": ");
    client.print(state);
    if (state=="On") {
      long deEnergiseAfter = (deEnergiseAt[n] - millis())/1000;
      if (deEnergiseAfter < 5000) {
        client.print(" for a further ");
        client.print(deEnergiseAfter);
        client.print("s");
      }
    }
    client.println("<br />");
  }
  
  client.println("<h1>Log History</h1>");
  client.println("<table>");
  for(int i = 0; i<LOG_ITEMS; i++) {
    int n = logPointer - i - 1;
    if (n<0) { n = LOG_ITEMS + n; }
    if (logMessages[n].length()>0) {
      client.print("<tr><td>");
      client.print(i);
      client.print('.');
      client.print("</td><td>");
      client.print((millis() - logTimes[n])/1000);
      client.print('s');
      client.print("</td><td>");
      client.print(logMessages[n]);
      client.print("</td></tr>");
    }
  }
  client.println("</table>");
  
  client.println("</html>");
  return "200 OK";
}

long getSwitchNumberFromQueryString(String request) {
  String switchNumber  = extractValueFromQueryString(request, "switch");
  long energiseNumber = -1; 
  if (switchNumber=="0") {
    energiseNumber = 0;
  } else {
    energiseNumber = switchNumber.toInt();
    if (energiseNumber==0) {
      energiseNumber = -1;
    }
  }
  if (energiseNumber>=SWITCHES) {
    energiseNumber = -1;
  }
  return energiseNumber;
}

long getDurationFromQueryString(String request) {
  String duration  = extractValueFromQueryString(request, "duration");
  long energiseFor = DEFAULT_ENERGISE_DURATION;
  if (duration.length()!=0) {
    energiseFor = duration.toInt();
  }
  return energiseFor;
}

void sendKeepAlive(unsigned long currentTime) {
  udp("switcher_alive", "1");
  nextDeviceKeepAliveTime = currentTime + KEEP_ALIVE_INTERVAL;
}

String ipAddressToString(IPAddress ipAddress) {
  String retval = "";
  retval.concat(ipAddress[0]);
  retval.concat('.');
  retval.concat(ipAddress[1]);
  retval.concat('.');
  retval.concat(ipAddress[2]);
  retval.concat('.');
  retval.concat(ipAddress[3]);
  return retval;
}

// A note on overloading: booleans are confused with ints.
void logC(String m)            { currentLine.concat(m); }
void logC(int m)               { currentLine.concat(m); }
void logC(long m)              { currentLine.concat(m); }
void logC(unsigned long m)     { currentLine.concat(m); }
void logC(IPAddress ipAddress) { currentLine.concat(ipAddressToString(ipAddress)); }

void logLn(String m)             { currentLine.concat(m); logClearLine(); }
void logLn(int m)                { currentLine.concat(m); logClearLine(); }
void logLn(long m)               { currentLine.concat(m); logClearLine(); }
void logLn(unsigned long m)      { currentLine.concat(m); logClearLine(); }
void logLn(IPAddress ipAddress)  { currentLine.concat(ipAddressToString(ipAddress)); logClearLine(); } 

void logClearLine() {
  logMessages[logPointer] = currentLine;
  logTimes[logPointer]    = millis();
  if (DEBUG) { Serial.println(currentLine); }
  logPointer = logPointer + 1;
  currentLine = "";
  if (logPointer==LOG_ITEMS) { logPointer = 0; }
}

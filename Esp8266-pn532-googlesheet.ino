#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
//#include <Adafruit_PN532.h>
#include "HTTPSRedirect.h"
#include "DebugMacros.h"
#include <Time.h>
#include <coredecls.h>


ESP8266WebServer server(80);

const int led_permit_read = 5; 
const int led_rfid_read = 03; 
const int led_rfid_denied = 14; 
const int led_rfid_accepted = 12;
const int led = LED_BUILTIN; 
const int lock = 13; 
const int pin_SDA = 4; 
const int pin_SCL = 0; 
PN532_I2C pn532spi(Wire); 
PN532 nfc(pn532spi);  

unsigned long previousMillisNFC = 0; 
unsigned long previousMillisPermissions = 0; 

#define ARRAYSIZE 100 
String idPermitted[ARRAYSIZE];



const long msOpening = 1500;  //ms tiempo de apertura de la lock
const long intervalNFC = 100;   // ms intervalo de chequeo de tarjeta
const long intervalPermissions = 1*60000;   // ms intervalo de chequeo de permisos

//const char *ssid = "dlink";
//const char *password = "03101993";

const char *ssid = "Totalcommander";
const char *password = "Totalo90";

const char* host = "script.google.com";
const int httpsPort = 443;
//https://script.google.com/macros/s/AKfycbyjlKVzlzjH0XL0SRD57f-O6ocs81pnpbG3DUad4Cv3yDWLKA8/exec
String url = "/macros/s/AKfycbyjlKVzlzjH0XL0SRD57f-O6ocs81pnpbG3DUad4Cv3yDWLKA8/exec?page=";

bool sntp_time_is_set = false;

bool got_ntp = false;

 HTTPSRedirect* client = nullptr;

// echo | openssl s_client -connect script.google.com:443 |& openssl x509 -fingerprint -noout
 const char* fingerprint = "8D:B7:8B:3F:96:EA:1C:44:89:01:B0:CD:4A:B5:35:FB:0C:F1:D9:E5";
//const uint8_t fingerprint[20] = {};

// Definition der Debuglevel
#define DEBUG_ERROR 1
#define DEBUG_WARNING 2
#define DEBUG_MIN_INFO 3
#define DEBUG_MED_INFO 4
#define DEBUG_MAX_INFO 5

int debug = 5;

int k = 0;

void openDoor(void){
    digitalWrite ( led, 0 );
    const long n = (int)msOpening/10;
    for(int i = 0; i < n; i++){
      digitalWrite ( lock, 1 );
      delay(5);
      digitalWrite ( lock, 0 );
      delay(5);
    }
    digitalWrite ( led, 1 );
    digitalWrite ( lock, 0 );
}


String PrintHex(const byte * uid, const long uidLength){
  String r = "";
  r+= uid[0];
  for (uint8_t i=1; i < uidLength; i++){
      r += "-";
      r += uid[i]&0xff; 
  }
  return r;
}

String searchNFC(void) {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 (Mifare Classic) or 7 (Mifare Ultralight) bytes depending on ISO14443A card type)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  if (success) 
    return PrintHex(uid,uidLength);
  return "0";
}

void verNFC(void){
  String uid;
  uid = searchNFC();
  if(uid != "0"){  // Display some basic information about the card
    Serial.println(" ");
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Value: ");
    Serial.print(uid);
    Serial.println("");
    digitalWrite ( led_rfid_read, 0 );//lit
  //got_ntp = acquireNetworkTime();
  //debug_out(F("\nNTP time "), DEBUG_MIN_INFO, 0);
  //debug_out(String(got_ntp?"":"not ")+F("received"), DEBUG_MIN_INFO, 1);
    bool notFound = true;
    for(int i = 0; i < ARRAYSIZE; i++){
      if(uid == idPermitted[i]){

      digitalWrite ( led_rfid_accepted, 0 );//lit
  
         // if(uid == "35-107-94-27"){
        openDoor();
        logEntry(uid);
        notFound = false;
        break;
      }
    }
    if(notFound)// read id, but it was not in permit list
    
      postAccess(url + "error&id=" + uid );
      digitalWrite ( led_rfid_denied, 0 );//lit
      
  } else {
    digitalWrite ( led_rfid_read, 1 );//unlit
    digitalWrite ( led_rfid_denied, 1 );//unlit
    digitalWrite ( led_rfid_accepted, 1 );//unlit
    }
}


void logEntry(String id){
    Serial.println(" ");
    Serial.println("Access granted to: " + id);
    Serial.println(url + "access&id=" + id);
    postAccess(url + "access&id=" + id );
}

void postAccess(String url){
  HTTPSRedirect* client = nullptr;
  Serial.println(" ");
  Serial.print("Posting data: "); Serial.println(url);

  client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
  client->setPrintResponseBody(false);
  client->setContentTypeHeader("application/json");
  Serial.print("postAccess(String url) Connecting to ");
  Serial.println(host);

  bool flag = false;
  for (int i=0; i<5; i++){   // Try to connect for a maximum of 5 times
    int retval = client->connect(host, httpsPort);
    Serial.print("retval = client->connect(host, httpsPort);: "); Serial.println(retval);

    if (retval == 1) {
       flag = true;
       Serial.println("retval = 1, Connected ");
       break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }

  if (!flag){
    Serial.println("Could not connect to server");
    delete client;
    client = nullptr;
    return;
  }
  
  Serial.println("\nGET: Sending Google Data:");
  Serial.println("================================");
  client->GET(url, host); // posting data
  delete client;  // delete HTTPSRedirect object
  client = nullptr;
  return ;
}



void refreshPermissions(void) {
  int pos = 0;
  int i = 0;
  String res = getPermissions();
  Serial.println(" ");
  if (res.indexOf(",") < 2) {
  
  res = "XXXXXXXXXXXXXXXXXXX,";
  }
  Serial.println("************************");
  Serial.println("* Permisions: ");
    while(res.indexOf(",", pos) > 0){
      
    idPermitted[i] = res.substring(pos, res.indexOf(",", pos));// fill fresh permits
    pos = res.indexOf(",", pos) +1;
    Serial.println(idPermitted[i]);
    i++;
  }
  for(int j = i; j < ARRAYSIZE; j++)// claen the rest of the list
  //  for(var j = i; j < ARRAYSIZE; j++)
      idPermitted[j] = "";
  Serial.println("************************");
}

String getPermissions(){
  HTTPSRedirect* client = nullptr;
  String n_url = url + "permissions";
  client = new HTTPSRedirect(httpsPort);
   client->setInsecure();// from logging
  client->setPrintResponseBody(true);// was false
  client->setContentTypeHeader("application/json");
  
  Serial.print("getPermissions() Connecting to ");
  Serial.println(host);

  bool flag = false;  // Try to connect for a maximum of 5 times
  for (int i=0; i<5; i++){
    int retval = client->connect(host, httpsPort);
    Serial.print("retval = client->connect(host, httpsPort);: "); Serial.println(retval);
    if (retval == 1) {
      Serial.println("retval = 1, Connected ");
       flag = true;
       break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }

  if (!flag){
    Serial.println("Could not connect to server.");
    delete client;
    client = nullptr;
    return "";
  }

/*
  if (client->setFingerprint(fingerprint)) {//xxxxxxxxxxxx
    Serial.println("Certificate match.");
  } else {
    Serial.println("Certificate mis-match");
  }
*/
  Serial.println("\nGET: Fetch Google Data:");
  Serial.println("================================");
  if(client->GET(n_url, host)){  
    String resp = client->getResponseBody();
    Serial.print("resp = ");
    Serial.println(resp);
    delete client;
    client = nullptr;
    if( sizeof(resp) > 1) digitalWrite(led_permit_read, 0);// lit
    return resp;
  }
  delete client;  // delete HTTPSRedirect object
  client = nullptr;
  return "";
}



void handleRoot() {
char temp[400];
int sec = millis() / 1000;
int min = sec / 60;
int hr = min / 60;
snprintf ( temp, 400,
"<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>Control de Acceso</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
  </body>\
</html>",

hr, min % 60, sec % 60
);
server.send ( 200, "text/html", temp );
}

void handleNotFound() {
 String message = "File Not Found\n\n";
 message += "URI: ";
 message += server.uri();
 message += "\nMethod: ";
 message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
 message += "\nArguments: ";
 message += server.args();
 message += "\n";
 for ( uint8_t i = 0; i < server.args(); i++ ) {
 message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
}
server.send ( 404, "text/plain", message );
}

void handleOpen() {
  server.send ( 200, "text/html", "OK" );
  //openDoor();
  if(server.args()){
    bool notFound = true;
    for(int i = 0; i < ARRAYSIZE; i++){
      if(server.arg(0) == idPermitted[i]){
        openDoor();
        logEntry(server.arg(0));
        notFound = false;
        break;
      }
    }
    if(notFound)
      postAccess(url + "error&id=" + server.arg(0) );
  }
}


String macToStr(const uint8_t* mac){
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}


void time_is_set (void) {
  sntp_time_is_set = true;
}

static bool acquireNetworkTime() {
  int retryCount = 0;
  debug_out(F("Setting time using SNTP"), DEBUG_MIN_INFO, 1);
  time_t now = time(nullptr);
  debug_out(ctime(&now), DEBUG_MIN_INFO, 1);
  debug_out(F("NTP.org:"),DEBUG_MIN_INFO,1);
  settimeofday_cb(time_is_set);
  configTime(8 * 3600, 0, "pool.ntp.org");
  while (retryCount++ < 20) {
    // later than 2000/01/01:00:00:00
    if (sntp_time_is_set) {
      now = time(nullptr);
      debug_out(ctime(&now), DEBUG_MIN_INFO, 1);
      return true;
    }
    delay(500);
    debug_out(".",DEBUG_MIN_INFO,0);
  }
  debug_out(F("\nrouter/gateway:"),DEBUG_MIN_INFO,1);
  retryCount = 0;
  configTime(0, 0, WiFi.gatewayIP().toString().c_str());
  while (retryCount++ < 20) {
    // later than 2000/01/01:00:00:00
    if (sntp_time_is_set) {
      now = time(nullptr);
      debug_out(ctime(&now), DEBUG_MIN_INFO, 1);
      return true;
    }
    delay(500);
    debug_out(".",DEBUG_MIN_INFO,0);
  }
  return false;
}



/*****************************************************************
 * Debug output                                                  *
 *****************************************************************/
void debug_out(const String& text, const int level, const bool linebreak) {
  if (level <= debug) {
    if (linebreak) {
      Serial.println(text);
    } else {
      Serial.print(text);
    }
  }
}


void setup(void) {
  String clientMac = "";
  unsigned char mac[6];
  WiFi.macAddress(mac);
  clientMac += macToStr(mac);

  Serial.begin(115200);
  Serial.println("Hello!");

  Serial.println(clientMac);
 Serial.print ( "WiFi.getMode = " );
  Serial.println(WiFi.getMode());

pinMode ( led_permit_read, OUTPUT );
  digitalWrite(led_permit_read, 1);// unlit

pinMode ( led_rfid_denied, OUTPUT );
digitalWrite ( led_rfid_denied, 1 );//unlit
pinMode ( led_rfid_accepted, OUTPUT );
    digitalWrite ( led_rfid_accepted, 1 );//unlit
  pinMode ( led_rfid_read, OUTPUT );
  digitalWrite ( led_rfid_read, 1 );//unlit
  pinMode ( led, OUTPUT );
  digitalWrite ( led, 1 );
  pinMode ( lock, OUTPUT );
  digitalWrite ( lock, 0 );

  pinMode (pin_SDA, OUTPUT );
  pinMode (pin_SCL, OUTPUT );
  digitalWrite(pin_SDA, 1);
  digitalWrite(pin_SCL, 1);
  delay(10);
  digitalWrite(pin_SDA, 0);
  digitalWrite(pin_SCL, 0);
  delay(100);
  Wire.begin(pin_SDA,pin_SCL);
  nfc.begin();

  digitalWrite ( led, 0 );//changed
  uint32_t versiondata = nfc.getFirmwareVersion();
  int count = 0;
  while(! versiondata && count < 5) {
    Serial.println("Didn't find PN53x board");
    delay(1000);
    versiondata = nfc.getFirmwareVersion();
    count ++;
  }
  if(versiondata)
   Serial.print ( "versiondata:" );
  Serial.println ( versiondata );
    Serial.println("NFC ok");
  digitalWrite ( led, 1 );
  nfc.SAMConfig();
  nfc.setPassiveActivationRetries(0x02);

  WiFi.begin ( ssid, password );
  Serial.println ( "" );
  delay ( 500 );  // Wait for connection
  Serial.println ( "" );
  Serial.print ( "Connected to " );
  Serial.println ( ssid );
  delay(500);
   Serial.print ( "WiFi.getMode = " );
   Serial.println(WiFi.getMode());
  Serial.print ( "IP address: " );
//  Serial.println ( WiFi.localIP() );
  Serial.println ( WiFi.localIP().toString() ); 
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  server.on ( "/", handleRoot );
  server.on ( "/open", handleOpen );
  server.on ( "/updatePermissions", []() {
    previousMillisPermissions = millis() - intervalPermissions;
    server.send ( 200, "text/plain", "OK" );
  } );
  server.onNotFound ( handleNotFound );
  server.begin();
  Serial.println ( "HTTP server started" );
   Serial.print ( "WiFi.getMode = " );
 Serial.println(WiFi.getMode());
  previousMillisPermissions = millis() - intervalPermissions;
}

void loop ( void ) {
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillisNFC >= intervalNFC) {
    previousMillisNFC = currentMillis; 
    Serial.print(".");
    k = k +1;
    if(k == 100){
      Serial.println(".");
      k = 0;
      } 
   // Serial.println("Checking verNFC()");
    verNFC();
  }
  if(currentMillis - previousMillisPermissions >= intervalPermissions) {
    previousMillisPermissions = currentMillis; 
    Serial.println("Checking Permissions");
    refreshPermissions();
  }
  server.handleClient();
}

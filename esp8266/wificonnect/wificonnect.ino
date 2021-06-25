#include <ESP8266WiFi.h> // Include the Wi-Fi library
#include <ESP8266SSDP.h> // Include ssdp library
#include <ESP8266HTTPClient.h> // Include HTTP client library
#include <ArduinoJson.h> // Include the JSON library
#include "DHT.h" // Include the Temperature Sensor library

// Below libraries are required for UDP Support
#include "lwip/opt.h"
#include "lwip/udp.h"
#include "lwip/inet.h"
#include "lwip/igmp.h"
#include "lwip/mem.h"
#include "include/UdpContext.h"

// For the Temperature 
#define DHTPIN 4     // what digital pin the DHT22 is conected to
#define DHTTYPE DHT22   // there are multiple kinds of DHT sensors

// For the SSDP support 
#define SSDP_MULTICAST_ADDR 239, 255, 255, 250
#define SSDP_PORT 1900
#define SSDP_MULTICAST_TTL 2
#define SSDP_BUFFER_SIZE 1064

// Initializing the temperature sensor library
DHT dht(DHTPIN, DHTTYPE);

// For the temperature sensor
float humidity = 0;
float temperatureC = 0;
float temperatureF = 0;
float heatindexF = 0;
float heatindexC = 0;
bool isthereadhtsensor = true;
bool validdhtsensordata = false;

// For the dust sensor
const int measurePin = A0;
float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;  
bool isthereadustsensor = true;
bool validdustsensordata = false;

// For the Wifi connection & UDP, need to change this to 
const char* host     = "espmon";
const char* ssid     = "vivek_iot";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "9845159712";     // The password of the Wi-Fi network
unsigned int udpPort = 1900;
UdpContext* _UDPserver = nullptr;
IPAddress udpAddr(239, 255, 255, 250);
IPAddress subnet;
static const char _ssdp_search_template[] PROGMEM =
  "M-SEARCH * HTTP/1.1\r\n"
  "HOST: 239.255.255.250:1900\r\n"
  "MAN: \"ssdp:discover\"\r\n"
  "ST: rpi-esp-monitor\r\n"
  "MX: 1\r\n"
  "\r\n";

// For receiving the broadcast data and sending the appropriate respose
String apiKey = "MY_API_KEY";
bool validNotify = false;
bool gothostval = false;
String hostval = "";
bool gotntval = false;
String ntval = "";
bool gotntsval = false;
String ntsval = "";
bool gotusnval = false;
String usnval = "";
bool gotlocationval = false;
String locationval = "";
bool gotalval = false;
String alval = "";
String extraValues = "";
bool received_data = false;
bool processed_data = false;

// Set for debugging
bool isproduction = false;

void setup() {

  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  dht.begin();

  waitforsometime(2);

  if (!isproduction) Serial.println("DEBUG: Waking UP!!!");
  
  if (!isproduction) Serial.printf("DEBUG: Connection status: %d\n", WiFi.status());
  if (!isproduction) Serial.printf("DEBUG: Connecting to %s\n", ssid);
  
  // We dont need to reset the hostname
  if (!isproduction) Serial.printf("DEBUG: Default hostname: %s\n", WiFi.hostname().c_str());
  WiFi.hostname(host);
  if (!isproduction) Serial.printf("DEBUG: New hostname: %s\n", WiFi.hostname().c_str());
  
  WiFi.begin(ssid, password);             // Connect to the network
  Serial.print("INFO: Connecting to ");
  Serial.print(ssid); Serial.println(" ...");

  //While it is connecting, collect all the sensor data...
  if(isthereadhtsensor) {
    if(!readhtsensors()) {
      Serial.print("WARNING: No DHT sensor data");
    }    
  }

  if(isthereadustsensor) {
    if(!readdustsensors()) {
      Serial.print("WARNING: No Dust sensor data");
    }
  }

  int timewaited = 0;
  int maximumwaitingtime = 0;

  // Try to connect to the WIFI
  maximumwaitingtime = 60; // Try for a minute
  timewaited = 0;

  if (!isproduction) Serial.print("DEBUG: ");

  while (WiFi.status() != WL_CONNECTED && timewaited < maximumwaitingtime) { // Wait for the Wi-Fi to connect
    //put a wait for maximum 1 minute, else, sleep and try again after a while.
    delay(1000);
    if (!isproduction) Serial.print(++timewaited);
    if (!isproduction) Serial.print(' ');
  }
  if (!isproduction) Serial.println("...");

  // Still not connected, then only refresh the data for the display and go back to deep sleep, but only sleep for 60 seconds.
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("WARNING: No WiFi connection, only displaying the data");

    // We have all the sensor data, simply display it...
    printdhtsensordata();
    printdustsensordata();

    //Display in the oled before deep sleep.
    
    deepsleepforsometime(60e6);
  }
  
  IPAddress local = WiFi.localIP();
  IPAddress mcast(SSDP_MULTICAST_ADDR);  

  Serial.printf("INFO: Connecting to %s, with IP Address ",ssid);
  Serial.println(local);
  
  //wait for 3 seconds
  waitforsometime(3);



  if (igmp_joingroup(local, mcast) != ERR_OK ) {
    Serial.println("WARNING: SSDP failed to join igmp group\n");
  } else {
    //We want to retry 5 times to get the ssdp info, before giving up
    int retrycount = 0;
    processed_data = false;

    while (processed_data == false & retrycount < 5) {

      resetssdpreceivedvalues();
      _UDPserver = new UdpContext;
      _UDPserver->ref();

      if (!_UDPserver->listen(IP_ADDR_ANY, SSDP_PORT)) {
        Serial.println("WARNING: SSDP Listen Failed\n");
      } 
      else {
        _UDPserver->setMulticastInterface(local);
        _UDPserver->setMulticastTTL(SSDP_MULTICAST_TTL);
        _UDPserver->onRx(handleReceive);
        if (!_UDPserver->connect(mcast, SSDP_PORT)) {
          Serial.println("WARNING: SSDP Connect Failed\n");
        } 
        else {
          char buffer[1460];
          char valueBuffer[strlen_P(_ssdp_search_template) + 1];
              
          strcpy_P(valueBuffer, _ssdp_search_template);
    
          int len = snprintf_P(buffer, sizeof(buffer), _ssdp_search_template);

          Serial.print("INFO: Broadcasting Attempt Count : ");
          Serial.println(retrycount);
          Serial.println(buffer);
                
          _UDPserver->append(buffer, len);
          _UDPserver->send(IPAddress(SSDP_MULTICAST_ADDR), SSDP_PORT);
    
          //Wait till you get a response or timeout to 30 seconds.
          int i = 0;
          received_data = false;
          while (!received_data && i < 30) {
            delay(1000);
            Serial.print(++i); Serial.print(' ');
          }
          Serial.println(" ");
                  
          if(!received_data) {
            Serial.println("WARNING: Did not receive any data");
          }
          else {     
            ntval.trim();      
            if(ntval == "rpi-esp-monitor")
              processed_data = true;                            
          }
        }
      }
      
      //Cleanup
      _UDPserver->disconnect();
      delete _UDPserver;
      _UDPserver = nullptr;  

      retrycount++;
      //Wait for 5 seconds before retrying
      waitforsometime(5);
    }

    igmp_leavegroup(local, mcast);
  }

  // Wait for 10 Seconds before moving ahead...
  waitforsometime(10);    

  if(received_data == true && processed_data == true & locationval.length() > 5) {                
                
    Serial.println("INFO: Attempting to Send JSON input");
    HTTPClient  http;
    String servername = locationval+"/hereisdata";              
    String httpRequestData = "";

    DynamicJsonDocument doc(1024);

    doc["api_key"] = apiKey;
    doc["macadd"] = WiFi.macAddress();
    doc["ipaddr"] = WiFi.localIP();

    if(validdhtsensordata == true) {
      doc["temphumid"]["type"] = DHTTYPE;
      doc["temphumid"]["tempc"] = temperatureC;
      doc["temphumid"]["tempf"] = temperatureF;
      doc["temphumid"]["heatic"] = heatindexC;
      doc["temphumid"]["heatif"] = heatindexF;
      doc["temphumid"]["humidity"] = humidity;
    }
    if(validdustsensordata == true) {
      doc["dustsens"]["type"] = "GP2Y1010AU0F";
      doc["dustsens"]["vo"] = voMeasured;
      doc["dustsens"]["calcvo"] = calcVoltage;
      doc["dustsens"]["ddens"] = dustDensity;
    }

    int data_length = serializeJson(doc, httpRequestData);

    if (!isproduction) Serial.print("DEBUG: Server URL: ");
    if (!isproduction) Serial.println(servername);
    if (!isproduction) Serial.println("DEBUG: Sending JSON");
    if (!isproduction) Serial.println(httpRequestData);             
      
    http.begin(servername);
    http.addHeader("Content-Type", "application/json");
                
    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);

    Serial.print("INFO: HTTP Response code: ");
    Serial.println(httpResponseCode);

    if(httpResponseCode > 0) {
      //Get the request response payload
      String payload = http.getString();
      //Print the response payload
      if (!isproduction) Serial.println("DEBUG: Received JSON");
      if (!isproduction) Serial.println(payload);                  
    }
        
    // Free resources
    http.end();                
  }
  else {
    Serial.println("WARNING: Did not receive any server information");
  }
      
  //Deep sleep mode for 30 seconds, the ESP8266 wakes up by itself when GPIO 16 (D0 in NodeMCU board) is connected to the RESET pin
  //Serial.println("Going into deep sleep mode for 30 seconds");

  //600e6 for 10 minutes, current 30 seconds
  //ESP.deepSleep(30e6); 

  //5 Minute sleep
  deepsleepforsometime(300e6);
  
}

// Deep Sleeps for a specified amount of time
void deepsleepforsometime(unsigned long microseconds) {
  Serial.print("INFO: Going into deep sleep mode for ");
  Serial.print(microseconds);
  Serial.println(" micro seconds");
  
  ESP.deepSleep(microseconds);
}

void resetssdpreceivedvalues(){
  
  //Resetting all values
  validNotify = false;
  gothostval = false;
  hostval = "";
  gotntval = false;
  ntval = "";
  gotntsval = false;
  ntsval = "";
  gotusnval = false;
  usnval = "";
  gotlocationval = false;
  locationval = "";
  gotalval = false;
  alval = "";
  extraValues = "";
}

/* Function to wait for a number of seconds, doing the delays differently */
void waitforsometime(int timeinseconds) {
  int waittime = 0;

  Serial.print("INFO: Waiting for ");
  Serial.print(timeinseconds);
  Serial.println(" seconds...");
  while (waittime < timeinseconds) {
    delay(1000);
    Serial.print(++waittime); Serial.print(' ');
  }

  Serial.println("\nINFO: Continuing...\n");
}

// Print the DHT Sensor data
void printdhtsensordata() {
  
  if(validdhtsensordata) {
    Serial.println("INFO: Temperature & Humidity Sensor");
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print(", Temperature: ");
    Serial.print(temperatureC);
    Serial.print("*C, ");
    Serial.print(temperatureF);
    Serial.println("*F");
    Serial.print("Heat index: ");
    Serial.print(heatindexC);
    Serial.print("*C, ");
    Serial.print(heatindexF);
    Serial.println("*F");
  }
}

//Reads the DHT sensor data and populates the variables
bool readhtsensors() {

  bool returnvalue = false;
  
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temperatureC = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  temperatureF = dht.readTemperature(true);
      
  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperatureC) || isnan(temperatureF)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    returnvalue = true;
    // Compute heat index in Fahrenheit (the default)
    heatindexF = dht.computeHeatIndex(temperatureF, humidity);
    // Compute heat index in Celsius (isFahreheit = false)
    heatindexC = dht.computeHeatIndex(temperatureC, humidity, false);

    // No need to print it while reading the data
    // printdhtsensordata();
  }

  validdhtsensordata = returnvalue;

  return returnvalue;
}

void printdustsensordata () {

  if(validdustsensordata) {
    Serial.println("INFO: Dust Sensor");
    Serial.print("Raw Signal Value (0-1023): ");
    Serial.print(voMeasured);
    
    Serial.print(", Voltage: ");
    Serial.print(calcVoltage);
    
    Serial.print(", Dust Density: ");
    Serial.println(dustDensity);      
  }

}

//Reads the DHT sensor data and populates the variables
bool readdustsensors() {

  bool returnvalue = true;
  
  //read the dust sensor signal
  voMeasured = analogRead(measurePin);
  calcVoltage = voMeasured*(5.0/1024);
  dustDensity = 0.17*calcVoltage-0.1;
  
  if ( dustDensity < 0) {
    dustDensity = 0.00;
  }

  validdustsensordata = returnvalue;

  return returnvalue;
}

void analyzeBuffer(char * line_buffer) {
  
  String mybuffer = String(line_buffer);

  if (validNotify == false) {    
    if(mybuffer.startsWith("NOTIFY")) {
      validNotify = true;
      if (!isproduction) Serial.println("DEBUG: Received a NOTIFY response");
      return;
    }    
  }
  
  if (gothostval == false) {
    if(mybuffer.startsWith("HOST:")) {
      gothostval = true;
      //extract the host val
      hostval = mybuffer.substring(5);
      if (!isproduction) Serial.print("DEBUG: Received the hostval: ");
      if (!isproduction) Serial.println(hostval);
      return;
    }      
  }

  if (gotntval == false) {
    if(mybuffer.startsWith("NT:")) {
      gotntval = true;
      //extract the nt val
      ntval = mybuffer.substring(3);
      if (!isproduction) Serial.print("DEBUG: Received the ntval: ");
      if (!isproduction) Serial.println(ntval);      
      return;
    }      
  }

  if (gotntsval == false) {
    if(mybuffer.startsWith("NTS:")) {
      gotntsval = true;
      //extract the nt val
      ntsval = mybuffer.substring(4);
      if (!isproduction) Serial.print("DEBUG: Received the ntsval: ");
      if (!isproduction) Serial.println(ntsval);      
      return;
    }      
  }

  if (gotusnval == false) {
    if(mybuffer.startsWith("USN:")) {
      gotusnval = true;
      //extract the nt val
      usnval = mybuffer.substring(4);
      if (!isproduction) Serial.print("DEBUG: Received the usnval: ");
      if (!isproduction) Serial.println(usnval);
      return;
    }      
  }

  if (gotlocationval == false) {
    if(mybuffer.startsWith("LOCATION:")) {
      gotlocationval = true;
      //extract the nt val
      locationval = mybuffer.substring(9);
      if (!isproduction) Serial.print("DEBUG: Received the locationval: ");
      if (!isproduction) Serial.println(locationval);
      return;
    }      
  }

  if (gotalval == false) {
    if(mybuffer.startsWith("AL:")) {
      gotalval = true;
      //extract the nt val
      alval = mybuffer.substring(3);
      if (!isproduction) Serial.print("DEBUG: Received the alval: ");
      if (!isproduction) Serial.println(alval);
      return;
    }      
  }
  extraValues.concat(mybuffer);
  extraValues.concat("\n");
}

void handleReceive () {
  char buffer[SSDP_BUFFER_SIZE] = {0};

  Serial.println("Received an input\n");

  typedef enum {METHOD, URI, PROTO, KEY, VALUE, ABORT} states;
  states state = METHOD;
    
  uint8_t cursor = 0;
  uint8_t cr = 0;

  char line_buffer[SSDP_BUFFER_SIZE] = {0};
  while (_UDPserver->getSize() > 0) {
    char c = _UDPserver->read();    
   
    //We dont want to pring it from here
    //Serial.print(c);
    
    if(c != '\n' && c != '\r') {
      //add to the line buffer
      line_buffer[cr] = c;

      cr++;
    }

    if(c =='\n') {
      //We have reached the end of a line
      //Analyze the line buffer and see what should be done
      line_buffer[cr] = NULL;

      analyzeBuffer(line_buffer);
      
      cr=0;
    }        
  }
  
  received_data = true;

  if(extraValues.length()>0) {
      if (!isproduction) Serial.print("DEBUG: Received the extravalues: ");
      if (!isproduction) Serial.println(extraValues);   
  }
  //Serial.print("\n");
}

//Loop which continuously runs on the 
void loop() { 

}

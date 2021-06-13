#include <ESP8266WiFi.h>        // Include the Wi-Fi library
#include <ESP8266SSDP.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

#include "DHT.h"

#include "lwip/opt.h"
#include "lwip/udp.h"
#include "lwip/inet.h"
#include "lwip/igmp.h"
#include "lwip/mem.h"
#include "include/UdpContext.h"

#define DHTPIN 4     // what digital pin the DHT22 is conected to
#define DHTTYPE DHT22   // there are multiple kinds of DHT sensors
#define SSDP_MULTICAST_ADDR 239, 255, 255, 250
#define SSDP_PORT 1900
#define SSDP_MULTICAST_TTL 2
#define SSDP_BUFFER_SIZE 1064

DHT dht(DHTPIN, DHTTYPE);

const char* host     = "espmon";
const char* ssid     = "vivek_iot";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "9845159712";     // The password of the Wi-Fi network
const int measurePin = A0;

String apiKey = "REPLACE_WITH_YOUR_API_KEY";
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

IPAddress udpAddr(239, 255, 255, 250);
IPAddress subnet;
unsigned int udpPort = 1900;
UdpContext* _UDPserver = nullptr;
bool received_data = false;
bool processed_data = false;

static const char _ssdp_search_template[] PROGMEM =
  "M-SEARCH * HTTP/1.1\r\n"
  "HOST: 239.255.255.250:1900\r\n"
  "MAN: \"ssdp:discover\"\r\n"
  "ST: rpi-esp-monitor\r\n"
  "MX: 1\r\n"
  "\r\n";
  

void setup() {

  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  dht.begin();

  waitforsometime(2);
  
  Serial.println("Waking UP!!!");
  
  Serial.printf("Connection status: %d\n", WiFi.status());
  Serial.printf("Connecting to %s\n", ssid);
  
  // We dont need to reset the hostname
  Serial.printf("Default hostname: %s\n", WiFi.hostname().c_str());
  WiFi.hostname(host);
  Serial.printf("New hostname: %s\n", WiFi.hostname().c_str());
  
  WiFi.begin(ssid, password);             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
  //put a wait for maximum 1 minute, else, sleep and try again after a while.
    delay(1000);
    Serial.print(++i); Serial.print(' ');
  }

  if( i == 60 ) {
    Serial.println("Unable to connect to the network");
  } else {

    resetssdpreceivedvalues();
  
    Serial.printf("\nConnection status: %d", WiFi.status());
    Serial.println("\nConnection established!");  
    Serial.print("IP address: \t");
    Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer
     
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);
    
    float hif = 0;
    float hic = 0;
    
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println("Failed to read from DHT sensor!");
    } else {
      // Compute heat index in Fahrenheit (the default)
      hif = dht.computeHeatIndex(f, h);
      // Compute heat index in Celsius (isFahreheit = false)
      hic = dht.computeHeatIndex(t, h, false);

      Serial.println("Temperature & Humidity Sensor");
      Serial.print("Humidity: ");
      Serial.print(h);
      Serial.print(", Temperature: ");
      Serial.print(t);
      Serial.print("*C, ");
      Serial.print(f);
      Serial.println("*F");
      Serial.print("Heat index: ");
      Serial.print(hic);
      Serial.print("*C, ");
      Serial.print(hif);
      Serial.println("*F");
    }

    float voMeasured = 0;
    float calcVoltage = 0;
    float dustDensity = 0;  
  
    //read the dust sensor signal
    voMeasured = analogRead(measurePin);
    calcVoltage = voMeasured*(5.0/1024);
    dustDensity = 0.17*calcVoltage-0.1;
  
    if ( dustDensity < 0) {
      dustDensity = 0.00;
    }

    Serial.println("Dust Sensor");
    Serial.print("Raw Signal Value (0-1023): ");
    Serial.print(voMeasured);
  
    Serial.print(", Voltage: ");
    Serial.print(calcVoltage);
  
    Serial.print(", Dust Density: ");
    Serial.println(dustDensity);  

    //wait for 3 seconds
    waitforsometime(3);
  
    IPAddress local = WiFi.localIP();
    IPAddress mcast(SSDP_MULTICAST_ADDR);  
  
    _UDPserver = new UdpContext;
    _UDPserver->ref();
  
    if (igmp_joingroup(local, mcast) != ERR_OK ) {  
        Serial.println("SSDP failed to join igmp group\n");
    }
    else {
        if (!_UDPserver->listen(IP_ADDR_ANY, SSDP_PORT)) {
          Serial.println("SSDP Listen Failed\n");
        }
        else {
            _UDPserver->setMulticastInterface(local);
            _UDPserver->setMulticastTTL(SSDP_MULTICAST_TTL);
            _UDPserver->onRx(handleReceive);
            if (!_UDPserver->connect(mcast, SSDP_PORT)) {
              Serial.println("SSDP Connect Failed\n");
            }
            else {
              char buffer[1460];
              char valueBuffer[strlen_P(_ssdp_search_template) + 1];
              
              strcpy_P(valueBuffer, _ssdp_search_template);
    
              int len = snprintf_P(buffer, sizeof(buffer), _ssdp_search_template);
  
              int retrycount = 0;
                
              while (processed_data == false & retrycount < 5) {
                //broadcast to UDP
                Serial.print("Broadcasting Attempt Count : ");
                Serial.println(retrycount);
                Serial.println(buffer);
                
                _UDPserver->append(buffer, len);
                _UDPserver->send(IPAddress(SSDP_MULTICAST_ADDR), SSDP_PORT);
    
                //Wait till you get a response or timeout to 30 seconds.
                i = 0;
                received_data = false;
                while (!received_data && i < 30) {
                  delay(1000);
                  Serial.print(++i); Serial.print(' ');
                }
                Serial.println(" ");
                  
                if(!received_data) {
                  Serial.println("Did not receive any data");
                }
                else {
                  ntsval.trim();      
                  if(ntsval == "rpi-esp-monitor")
                    processed_data = true;                            
                }
                  
                retrycount++;
              }

              // Wait for 10 Seconds before moving ahead...
              waitforsometime(10);
                
              //We have everything we need, send the data to the Flask server
              if(received_data == true && processed_data == true & locationval.length() > 5) {
                
                Serial.println("Attempting to Send JSON input");
                HTTPClient  http;
                String servername = locationval+"/hereisdata";
                
                String httpRequestData = "";

                DynamicJsonDocument doc(1024);

                doc["api_key"] = apiKey;
                doc["macadd"] = WiFi.macAddress();
                doc["ipaddr"] = WiFi.localIP();
                doc["temphumid"]["type"] = DHTTYPE;
                doc["temphumid"]["tempc"] = t;
                doc["temphumid"]["tempf"] = f;
                doc["temphumid"]["heatic"] = hic;
                doc["temphumid"]["heatif"] = hif;
                doc["temphumid"]["humidity"] = h;
                doc["dustsens"]["type"] = "GP2Y1010AU0F";
                doc["dustsens"]["vo"] = voMeasured;
                doc["dustsens"]["calcvo"] = calcVoltage;
                doc["dustsens"]["ddens"] = dustDensity;

                int data_length = serializeJson(doc, httpRequestData);
                Serial.println(httpRequestData);
                
                Serial.println(servername);
                http.begin(servername);
                http.addHeader("Content-Type", "application/json");
                
                //String httpRequestData = "{\"api_key\":\"" + apiKey + "\",\"field1\":\"" + String(random(40)) + "\"}";           
                // Send HTTP POST request
                int httpResponseCode = http.POST(httpRequestData);

                Serial.print("HTTP Response code: ");
                Serial.println(httpResponseCode);

                if(httpResponseCode > 0) {
                  //Get the request response payload
                  String payload = http.getString();
                  //Print the response payload
                  Serial.println(payload);                  
                }
        
                 // Free resources
                 http.end();                
              }

              //Wait for 5 seconds before we cleanup and reset
              waitforsometime(5);
              
              igmp_leavegroup(local, mcast);
              
              //Cleanup
              _UDPserver->disconnect();
              delete _UDPserver;
              _UDPserver = nullptr;  
            }
        }
    }
  }
      
  // Deep sleep mode for 30 seconds, the ESP8266 wakes up by itself when GPIO 16 (D0 in NodeMCU board) is connected to the RESET pin
  Serial.println("Going into deep sleep mode for 30 seconds");

  //600e6 for 10 minutes, current 30 seconds
  ESP.deepSleep(30e6); 
  
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
  received_data = false;
  processed_data = false;
}

/* Function to wait for a number of seconds, doing the delays differently */
void waitforsometime(int timeinseconds) {
  int waittime = 0;

  Serial.print("Waiting for ");
  Serial.print(timeinseconds);
  Serial.println(" seconds...");
  while (waittime < timeinseconds) {
    delay(1000);
    Serial.print(++waittime); Serial.print(' ');
  }

  Serial.println("\nContinuing...");
}

void analyzeBuffer(char * line_buffer) {
  
  String mybuffer = String(line_buffer);

  if (validNotify == false) {    
    if(mybuffer.startsWith("NOTIFY")) {
      processed_data = true;
      validNotify = true;
      Serial.println("Received a NOTIFY response");
      return;
    }    
  }
  
  if (gothostval == false) {
    if(mybuffer.startsWith("HOST:")) {
      gothostval = true;
      //extract the host val
      hostval = mybuffer.substring(5);
      Serial.print("Received the hostval: ");
      Serial.println(hostval);
      return;
    }      
  }

  if (gotntval == false) {
    if(mybuffer.startsWith("NT:")) {
      gotntval = true;
      //extract the nt val
      ntval = mybuffer.substring(3);
      Serial.print("Received the ntval: ");
      Serial.println(ntval);      
      return;
    }      
  }

  if (gotntsval == false) {
    if(mybuffer.startsWith("NTS:")) {
      gotntsval = true;
      //extract the nt val
      ntsval = mybuffer.substring(4);
      Serial.print("Received the ntsval: ");
      Serial.println(ntsval);      
      return;
    }      
  }

  if (gotusnval == false) {
    if(mybuffer.startsWith("USN:")) {
      gotusnval = true;
      //extract the nt val
      usnval = mybuffer.substring(4);
      Serial.print("Received the usnval: ");
      Serial.println(usnval);
      return;
    }      
  }

  if (gotlocationval == false) {
    if(mybuffer.startsWith("LOCATION:")) {
      gotlocationval = true;
      //extract the nt val
      locationval = mybuffer.substring(9);
      Serial.print("Received the locationval: ");
      Serial.println(locationval);
      return;
    }      
  }

  if (gotalval == false) {
    if(mybuffer.startsWith("AL:")) {
      gotalval = true;
      //extract the nt val
      alval = mybuffer.substring(3);
      Serial.print("Received the alval: ");
      Serial.println(alval);
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
   
    Serial.print(c);
    
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
  Serial.print("\n");
}

void loop() { 

}

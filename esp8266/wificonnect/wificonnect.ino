#include <ESP8266WiFi.h>        // Include the Wi-Fi library

const char* ssid     = "vivek_iot";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "9845159712";     // The password of the Wi-Fi network

void setup() {
  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');
  
  Serial.printf("Connection status: %d\n", WiFi.status());
  Serial.printf("Connecting to %s\n", ssid);
  
  Serial.printf("Default hostname: %s\n", WiFi.hostname().c_str());
  WiFi.hostname("TempESP-01");
  Serial.printf("New hostname: %s\n", WiFi.hostname().c_str());
  
  WiFi.begin(ssid, password);             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); Serial.print(' ');
  }
  Serial.printf("\nConnection status: %d\n", WiFi.status());
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer
}

void loop() { }

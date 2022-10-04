#include <WiFi.h>                 //Wifi module
#define SECRET_SSID "HampoWiFi21-2G"
#define SECRET_PASS "stepmikelind"
//WiFiServer server(80);            //Instantiate wifi server

void setup() {
  // put your setup code here, to run once:
  pinMode(13, OUTPUT);
  Serial.begin(9600);                                       //Serial port for debug
  WiFi.mode(WIFI_STA);
  char ssid[] = SECRET_SSID;                          //Set the wifi network
  char pass[] = SECRET_PASS;                          //Set the wifi password
  int status = WL_IDLE_STATUS;                        //Set initial wifi status
  while (status != WL_CONNECTED) {                    //Wait for wifi connection
    if (Serial) Serial.print("Connecting to ");       //Print debug info
    if (Serial) Serial.println(ssid);                 //Print network name
//    WiFi.config(ip);
    status = WiFi.begin(ssid, pass);                  //Begin wifi
    if (Serial) Serial.print("status = ");
    if (Serial) Serial.println(status);
    delay(3000);                                      //Wait 5 sec for wifi to connect - DHCP
  } 
  if (Serial) Serial.print("IP address: ");           //Print debug info
  if (Serial) Serial.println(WiFi.localIP());         //Print IP address that was assigned

}

void loop() {
  // put your main code here, to run repeatedly:
digitalWrite(13, HIGH);
if (Serial) Serial.print("On");  
delay(1000);
digitalWrite(13, LOW);
delay(100);
}

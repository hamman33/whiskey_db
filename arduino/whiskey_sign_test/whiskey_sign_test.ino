#include <Adafruit_Protomatter.h>
#include <WiFi.h>
#include <stdlib.h>                   //String library

#define SECRET_SSID 
#define SECRET_PASS 

  uint8_t rgbPins[]  = {4, 12, 13, 14, 15, 21};
  uint8_t addrPins[] = {16, 17, 25, 26};
  uint8_t clockPin   = 27; // Must be on same port as rgbPins
  uint8_t latchPin   = 32;
  uint8_t oePin      = 33;

Adafruit_Protomatter matrix(
 128, 4, 1, rgbPins, 4, addrPins, clockPin, latchPin, oePin, false);

WiFiClient client;
char server[] = "10.10.1.18";

void setup(void) {
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);
  char ssid[] = SECRET_SSID;                          //Set the wifi network
  char pass[] = SECRET_PASS;                          //Set the wifi password
                     //Wait for wifi connection
  if (Serial) Serial.print("Connecting to ");       //Print debug info
  if (Serial) Serial.println(ssid);                 //Print network name
//    WiFi.config(ip);
  int wstatus = WL_IDLE_STATUS; 
  wstatus = WiFi.begin(ssid, pass);                  //Begin wifi                            
  while (WiFi.waitForConnectResult() != WL_CONNECTED); //Wait for wifi to connect - DHCP
  if (Serial) Serial.print("IP address: ");           //Print debug info
  if (Serial) Serial.println(WiFi.localIP());  

  

  // Initialize matrix...
  ProtomatterStatus status = matrix.begin();
  Serial.print("Protomatter begin() status: ");
  Serial.println((int)status);
  if(status != PROTOMATTER_OK) {
    for(;;);
  }

  matrix.println("The Boys Whiskey Collection"); // Default text color is white

  // AFTER DRAWING, A show() CALL IS REQUIRED TO UPDATE THE MATRIX!

  matrix.show(); // Copy data to matrix buffers
}

void loop(void) {

  printRatings();
  
  
  Serial.print("Refresh FPS = ~");
  Serial.println(matrix.getFrameCount());
  delay(1000);
}

void printRatings(){
  Serial.println("Getting ratings:");
  char response[1024];
  int del[5];
  bool conf = querySite("recentratings", response, del);
  if(conf){
    for(int ypos = 0; ypos > -50; ypos--){
      matrix.fillScreen(0);
      matrix.setCursor(0, ypos);
      String s = String(response);
      char num[8];
      for(int i = 0; i<5; i++){
        sprintf(num, "%d. ", i+1);
        matrix.print(num);
        if(i == 0){
          matrix.println(s.substring(0,del[i]));
          Serial.println(s.substring(0,del[i]));
        }else{
          matrix.println(s.substring(del[i-1],del[i]));
          Serial.println(s.substring(del[i-1],del[i]));
        }
      }
      matrix.show();
      delay(400);
    }
    
  }
  
}

bool querySite(char* page, char* response, int* del){
  if (client.connect(server, 5000)) {
    Serial.println("connected to server");
    char getRequest[64];
    sprintf(getRequest, "GET /%s", page);
    client.println(getRequest);
    client.println();
  }
  delay(100);
  if(!client.available()){
    client.stop();
    return false;
  }else{
    int incomingChar = 0;
    int ratNum = 0;
    while (client.available()) {
      char c = client.read();
      if(c == ';'){
        del[ratNum] = incomingChar;
        ratNum++;
      }else{
        response[incomingChar] = c;
        //Serial.write(c);
        incomingChar++;
      }
    }
  }
  client.stop();
  return true;
}


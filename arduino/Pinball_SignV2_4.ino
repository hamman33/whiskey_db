//Pinball Sign V1_4
//Copyright Richard J. Hampo, 2021
// For Arduino Nano RP2040 Connect device - under Arduino Mbed OS boards

#include <SPI.h>                      //Needed for wifi
#include <WiFiNINA.h>                 //Wifi module
#include <TimeLib.h>                  //Handles time
#include <Timezone.h>                 //Converts time to proper timezone
#include <String.h>                   //String library
#include <stdlib.h>                   //String library
#include <Wire.h>                     //Needed for EEPROM
#include "SparkFun_External_EEPROM.h" // Click here to get the library: http://librarymanager/All#SparkFun_External_EEPROM

#define SPKR 17                        //Pin for speaker

#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define GREY  0x4208 //  (BLUE/16 + RED/16 + GREEN/16)
#define ORANGE 0xFA00

#define SECRET_SSID "HampoWiFi21-2G"
#define SECRET_PASS "stepmikelind"

//Definitions for EEPROM
#define EE_START 0
#define ADDRESS_NNS (EE_START)                                //Address to store number of new scores
#define ADDRESS_NHS (EE_START+sizeof(int))                    //Address to store number of high scores array
#define ADDRESS_NS (ADDRESS_NHS+3*sizeof(int))                //Address to store the new scores
#define ADDRESS_HS (ADDRESS_NS+NNS*sizeof(struct score))      //Address to store the high scores
#define ADDRESS_NPS (ADDRESS_HS + NHS*3*sizeof(struct score)) //Address to store the number of personal scores
#define ADDRESS_PHS (ADDRESS_NPS + sizeof(int))               //Address to store the personal scores
#define NNS 10            //Max number of new scores
#define NHS 10            //Max number of hih scores per machine
#define NPS 30            //Max number of personal high scores
#define ADDRESS_MESS 10000

//Define states
#define SH_SP 0         //Show splash screen during startup
#define SH_NS 1         //Show new score list
#define SH_HS 2         //Show high score list
#define SH_PS 3         //Show personal score list
#define SH_NE 4         //Show new entry
#define SH_ME 5         //Show Message
#define SH_WE 6         //Show Weather

//Define display messages
#define SETCURSOR 1       //first char is operation, then comma, then x, then comma, then y, then \n
#define SETTEXTCOLOR 2    //First char is operation type, then comma, then 2 bytes for color
#define PRINTLN 3         //First char is operation type, then comma,then text
#define SHOW 4
#define CLEARLN 5         //0,1,2,3 mean clear single line, 4 clears lines 1,2,3 and 5 clears 0,1,2,3,4
#define CURSCOLTEXT 6     //first char is operation, then comma, then x, then y, then 2 bytes color then text

//Function prototypes
void get_EE_scores();                                   //Read the scores from external EEPROM
void write_EE_scores();                                 //Write the scores to external EEPROM
void playnote(int pin, int freq, int duration);         //Play a tone 
void playfanfare();                                     //Play fanfare sound - for new high score
int xcenter(char *str);                                 //Return x coordinate for centering the string
char* date(time_t timesec);                             //Returns date string - 4 digit year
char* date2(time_t timesec);                            //Returns date string - 2 digit year
char *scorestring(unsigned long sc);                    //Create score string with comma separators
void showrecent(WiFiClient client);                     //Serve HTML code for drawing new score table
void showhigh(WiFiClient client);                       //Serve HTML code for drawing high score table
void showtitle(WiFiClient client);                      //Serve HTML code for printing page title
void showpers(WiFiClient client);                       //Serve HTML code for drawing personal score table
void serve_adminpage(WiFiClient client);                //Serve full admin page
void servescores(WiFiClient client);                    //Serve full scores page
void serve_inputpage(WiFiClient client, int gm);        //Serve a page for client to enter a high score
String parsetoken(String line, String beg, String en);  //Function to return substring with begin and end tokens
void addhighscore(int mach, String in, unsigned long sc, time_t t);     //Function to add a new high score
void addpersonalscore(int mach, String in, unsigned long sc, time_t t); //Function to add new personal score
void setmessage(String line);                           //Function to set message from client return string
void deletenewscore(String line);                       //Delete one of the new scores - from client return data
void changenewdate(String line);                        //Change date on a new score - from client return data
int gamenum(String gm);                                 //Return game number that matches input string
void deletehighscore(String line);                      //Delete one of the new scores - from client return data
void changehighdate(String line);                       //Change date on a high score - from client return data
void changepersdate(String line);                       //Change date on a personal score - from client return data
void deleteperscore(String line);                       //Delete one of the personal high scores - from client return data
void addnewscore(int mach, String in, unsigned long sc);//Add new score, calls add high, add personal, and write EEPROM
void clearscores(String line);                          //Function to clear scores - which type is based on return data from client
void serveclear(WiFiClient client);                     //Function to serve page with confirmation of clear scores
void serve_delnew(WiFiClient client);                   //Serve page to allow delete/change new score
void serve_delhigh(WiFiClient client);                  //Serve page to allow delete/change high score
void serve_delpers(WiFiClient client);                  //Serve page to allow delete/change personal score
void parseget(String line, WiFiClient client);          //Parse the client GET input line, take appropriate action, and serve next page
char* dir(int angle);                                   //Return string of wind direction
void getWeather();                                      //Get weather data
void service_wifi();                                    //Check if new wifi data and process it
void trans(char* line, int num);                        //Transmit data to display via I2C
void sendcolor(int color);                              //Send color
void sendcursor(int x, int y);                          //Send cursor position
void sendtext(char *text);                              //Send a text string
void sendclrln(int ln);                                 //Send clear line. 0-4 clears single line. 8 clears line 1,2,3. 9 clears 0,1,2,3
void sendshow();                                        //Send clear line
void sendcurscoltext(int x,int y,int color,char *text); //Send the coords, color and text all together
void writenew(int i, int index, int j);                 //Write one line of new score
void writehigh(int i, int index, int gm, int j);        //Write one line of high score
void writepers(int i, int index);                       //Write one line of personal high score
void writenewentry();                                   //Draws new score screen 
void writemess();                                       //Draws message screen
int tempcolor(float temp);                              //Returns color for temperature
int condcolor(int code);                                //Return color for weather condition
int wscolor(float ws);                                  //Return color for windspeed
void writewea(int i, int index, int j);                 //Write one line of weather. i=line number to draw on, indx = line of weathervoid j=scroll 
void title(char *text, int col, int x, int y);          //Draws title line given text, color, coords

// Global Variables
int state,indx,gm,scroll;             //Overall state and index and game of which item is being displayed
static unsigned long t_last, t_now;   //Time function
struct score {                        //Structure with scores
  int machine;                        //0=Medieval Madness, 1 = Twilight Zone, 2 = Jurassic Park
  char initials[4];                   //Initials
  unsigned long score;                //Score
  time_t t;                           //Time high score entered
} hs[NHS][3], ns[NNS];                //10 high scores for each machine and 10 newest scores. If score is zero, then that is end of list

struct phs {              //Personal high scores - one per machine
  char initials[4];       //Initials
  unsigned long score[3]; //One score per machine
  time_t t[3];            //One date per machine
} phs[NPS];

String Machine[3] = {"Medieval Madness", "Twilight Zone", "Jurassic Park"};
char MI[3][3]={"MM","TZ","JP"};                             //Machine Initials
int MC[3]={ORANGE, BLUE, GREEN};                            //Color of text for machine names
int nhs[3], nns, nps;                                       //number of high scores, number of new scores, personal high scores
time_t timesec;                                             //Time 
TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240};  //UTC - 4 hours
TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};   //UTC - 5 hours
Timezone usEastern(usEDT, usEST);                           //Declare this object to convert time to local
char string[80];                                            //String for debug printing
char line[80];                                              //string used for display I2C transmission
char L[4][40];                                              //Message lines

char *heading[16] = {"N  ","NNE","NE ","ENE","E  ","ESE","SE ","SSE","S  ","SSW","SW ","WSW","W  ","WNW","NW ","NNW"};
struct wea {
  float temp, feels, wsp;
  time_t sunrise,sunset;
  int hum,wang,code;
  String cond;
} wea;

ExternalEEPROM myMem;             //Instantiate EEPROM
WiFiServer server(80);            //Instantiate wifi server
IPAddress ip(192, 168, 1, 45);  //IP address
IPAddress dns(192,168,1,1);
IPAddress gw(192,168,1,1);
IPAddress subnet(255,255,254,0);

void get_EE_scores(){
  int i,j;
  myMem.get(ADDRESS_NNS, nns);                                        //Save num new scores
  for (i=0;i<nns;i++){                                                //Loop over new scores
    myMem.get((ADDRESS_NS+i*sizeof(struct score)), ns[i]);            //Save new score
  }
  for (j=0;j<3;j++){                                                  //Loop over different games
    myMem.get(ADDRESS_NHS+j*sizeof(int), nhs[j]);                     //Save num high scores
    for (i=0;i<nhs[j];i++){                                           //Loop over number of high scores
      myMem.get(ADDRESS_HS+(i+NHS*j)*sizeof(struct score), hs[i][j]); //Save high score
    }
  }
  myMem.get(ADDRESS_NPS, nps);                                        //Save num personal scores
  for (i=0;i<nps;i++){                                                //Loop over new scores
    myMem.get(ADDRESS_PHS+i*sizeof(struct phs), phs[i]);              //Save new score
  } 
  sprintf(string,"Reading EEPROM: nns=%d, nhs=%d,%d,%d",nns,nhs[0],nhs[1],nhs[2]);
  if (Serial) Serial.println(string);
}

void write_EE_scores(){
  int i,j;
  myMem.put(ADDRESS_NNS, nns);                                        //Save num new scores
  for (i=0;i<nns;i++){                                                //Loop over new scores
    myMem.put(ADDRESS_NS+i*sizeof(struct score), ns[i]);              //Save new score
  }
  for (j=0;j<3;j++){                                                  //Loop over different games
    myMem.put(ADDRESS_NHS+j*sizeof(int), nhs[j]);                     //Save num high scores
    for (i=0;i<nhs[j];i++){                                           //Loop over number of high scores
      myMem.put(ADDRESS_HS+(i+NHS*j)*sizeof(struct score), hs[i][j]); //Save high score
    }
  }
  myMem.put(ADDRESS_NPS, nps);                                        //Save num personal scores
  for (i=0;i<nps;i++){                                                //Loop over new scores
    myMem.put(ADDRESS_PHS+i*sizeof(struct phs), phs[i]);              //Save new score
  }
}

void playnote(int pin, int freq, int duration){
  tone(pin,freq,duration);
  delay(1.1*duration);
}

void playfanfare(){
  int duration=600;                                   //Duration of a whole note for fanfare
  playnote(SPKR,196,duration/4);
  playnote(SPKR,262,duration/4);
  playnote(SPKR,330,duration/4);
  playnote(SPKR,392,duration/2);
  playnote(SPKR,330,duration/4);
  playnote(SPKR,392,duration);  
}

int xcenter(char *str){                               //Return x coordinate for centering the string
  return (192-strlen(str)*6)/2;
}

void setup() {
  char ssid[] = SECRET_SSID;                          //Set the wifi network
  char pass[] = SECRET_PASS;                          //Set the wifi password

  pinMode(SPKR, OUTPUT);                              //Connection for speaker
  Serial.begin(9600);                                 //Serial port speed for debug info
//  while (!Serial);                                    //Wait until serial is ready
  Wire.begin();                                       //Start I2C
//  Wire.setClock( 400000L);
  delay(1000);                                        //Wait 1 second to let the display controller initialize - otherwise the I2C fails to start
  if (myMem.begin(0b1010100,Wire) == false)           //Memory set to adress 0x100 (last 3 bits)
  {
    if (Serial) Serial.println("No memory detected. Freezing.");  //Memory not working
    while (1);                                        //Wait forever
  }
  if (Serial) Serial.print("Memory Detected: Bytes: ");//Print message
  if (Serial) Serial.println(myMem.length());         //Print size of memory
  int status = WL_IDLE_STATUS;                        //Set initial wifi status
  while (status != WL_CONNECTED) {                    //Wait for wifi connection
    if (Serial) Serial.print("Connecting to ");       //Print debug info
    if (Serial) Serial.println(ssid);                 //Print network name
//    WiFi.config(ip);
    status = WiFi.begin(ssid, pass);                  //Begin wifi
    delay(5000);                                      //Wait 5 sec for wifi to connect - DHCP
  } 
  if (Serial) Serial.print("IP address: ");           //Print debug info
  if (Serial) Serial.println(WiFi.localIP());         //Print IP address that was assigned
    WiFi.config(ip,dns,gw,subnet);
  if (Serial) Serial.print("IP address: ");           //Print debug info
  if (Serial) Serial.println(WiFi.localIP());         //Print IP address that was assigned
  server.begin();                                     //Begin wifi server
  get_EE_scores();                                    //Read scores from EEPROM
  state=SH_SP;                                        //Initialize display state
  indx=-1;                                            //Set index (used by state) to -1 - this draws title
  t_last=millis();                                    //Set starting time
}

char* date(time_t timesec){                                                       //Function to create date string with 4 digit year
  sprintf(string,"%02d/%02d/%02d",month(timesec),day(timesec),year(timesec));     //Assemble the date
  return string;                                                                  //Return
}
char* date2(time_t timesec){                                                      //Function to create date string with 2 digit year
  sprintf(string,"%02d/%02d/%02d",month(timesec),day(timesec),year(timesec)%100); //Assemble the date
  return string;                                                                  //Return
}

char *scorestring(unsigned long sc){                                                                        //Create score string with comma separators
  static char str[20];                                                                                      //Temporary string variable
  if (sc < 1000) sprintf(str,"%13d",sc);                                                                    //Less than thousand
  else if (sc < 1000000) sprintf(str,"%9d,%03d",sc/1000, sc%1000);                                          //less than million
  else if (sc < 1000000000) sprintf(str,"%5d,%03d,%03d",sc/1000000, (sc%1000000)/1000, sc%1000);            //Less thab billion
  else sprintf(str,"%1d,%03d,%03d,%03d",sc/1000000000,(sc%1000000000)/1000000, (sc%1000000)/1000, sc%1000); //More than billion
  return str;                                                                                               //Return string
}

void showrecent(WiFiClient client){         //Serve HTML code for drawing new score table
  int i;
  client.println(F("<style>table,th,td{border: 1px solid black;}</style>"));
  client.println(F("<h2>Recent Scores</h2>"));
  client.println(F("<table><tr><th>#</th><th>Machine</th><th>Initials</th><th>Score</th><th>Date</th></tr>"));
  for (i=0;i<nns;i++) {
    client.println("<tr><td>"+String(i+1)+"</td><td>"+Machine[ns[i].machine]+"</td><td>"+ns[i].initials+"</td><td align=\"right\">"+scorestring(ns[i].score)+"</td><td>"+date(ns[i].t)+"</td></tr>");
  }
  client.println(F("</table>"));
}

void showhigh(WiFiClient client){           //Serve HTML code for drawing high score table
  int i;
  client.println(F("<style>table,th,td{border: 1px solid black;}</style>"));
  client.println(F("<h2>High Scores</h2>"));
  client.println(F("<table><tr><th></th><th colspan=\"3\">Medieval Madness</th><th colspan=\"3\">Twilight Zone</th><th colspan=\"3\">Jurassic Park</th></tr>"));
  client.println(F("<b><td>#</td><td>Initials</td><td>Score</td><td>Date</td><td>Initials</td><td>Score</td><td>Date</td><td>Initials</td><td>Score</td><td>Date</td></tr></b>"));
  for (i=0;i<max(max(nhs[0],nhs[1]),nhs[2]);i++) {
    client.println("<tr><td>"+String(i+1)+"</td>");
    if(i>= nhs[0]) client.println(F("<td></td><td></td><td></td>"));
    else client.println("<td>"+String(hs[i][0].initials)+"</td><td align=\"right\">"+scorestring(hs[i][0].score)+"</td><td>"+date(hs[i][0].t)+"</td>");
    if(i>= nhs[1]) client.println(F("<td></td><td></td><td></td>"));
    else client.println("<td>"+String(hs[i][1].initials)+"</td><td align=\"right\">"+scorestring(hs[i][1].score)+"</td><td>"+date(hs[i][1].t)+"</td>");
    if(i>= nhs[2]) client.println(F("<td></td><td></td><td></td></tr>"));
    else client.println("<td>"+String(hs[i][2].initials)+"</td><td align=\"right\">"+scorestring(hs[i][2].score)+"</td><td>"+date(hs[i][2].t)+"</td></tr>");
  }
  client.println(F("</table>"));  
}

void showtitle(WiFiClient client){           //Serve HTML code for printing page title
  client.println(F("<style>h1 {font-size: 40px;} input[type=submit] {width: 450px; font-size: 30px;}</style>"));
  client.println(F("<style>h2 {font-size: 30px;} </style><html>"));
  client.println(F("<h1><p><b><u>Hampo Arcade Pinball Scoreboard</u></b></p>"));  
}

void showpers(WiFiClient client){           //Serve HTML code for drawing personal score table
  int i;
  client.println(F("<style>table,th,td{border: 1px solid black;}</style>"));
  client.println(F("</table><h2>Personal High Scores</h2>"));
  client.println(F("<table><tr><th></th><th colspan=\"2\">Medieval Madness</th><th colspan=\"2\">Twilight Zone</th><th colspan=\"2\">Jurassic Park</th></tr>"));
  client.println(F("<tr><td>Initials</td><td>Score</td><td>Date</td><td>Score</td><td>Date</td><td>Score</td><td>Date</td></tr>"));
  for(i=0;i<nps;i++){
    if(phs[i].score[0] != 0) {
      client.println("<td>"+String(phs[i].initials)+"</td><td align=\"right\">"+scorestring(phs[i].score[0])+"</td><td>"+date(phs[i].t[0])+"</td>");
    } else {
      client.println("<td>"+String(phs[i].initials)+"</td><td></td><td></td>");
    }
    if(phs[i].score[1] != 0) {
      client.println((String)("<td align=\"right\">")+scorestring(phs[i].score[1])+"</td><td>"+date(phs[i].t[1])+"</td>");
    } else {
      client.println("<td></td><td></td>");
    }
    if(phs[i].score[2] != 0) {
      client.println((String)"<td align=\"right\">"+scorestring(phs[i].score[2])+"</td><td>"+date(phs[i].t[2])+"</td></tr>");
    } else {
      client.println("<td></td><td></td><tr>");
    }
  }
  client.println(F("</table>"));  
}

void serve_adminpage(WiFiClient client) {      //Serve full admin page 
  showtitle(client);
  client.println(F("<br><form action=\"http://192.168.1.45/DelNew\"><input type='submit' value=\"Delete/Modify New Score\"></form>"));
  client.println(F("<br><form action=\"http://192.168.1.45/DelHigh\"><input type='submit' value=\"Delete/Modify High Score\"></form>"));
  client.println(F("<br><form action=\"http://192.168.1.45/DelPers\"><input type='submit' value=\"Delete/Modify Personal Score\"></form>"));
  client.println(F("<form action=\"/Message\">Set Message: <br>"));
  client.println(F("<label for=\"Line1\">Line1:</label><input type=\"text\"style=\"font-size:30pt\"id=\"L_1\"name=\"L_1\"maxlength=\"32\"size=\"32\"><br>"));
  client.println(F("<label for=\"Line1\">Line2:</label><input type=\"text\"style=\"font-size:30pt\"id=\"L_2\"name=\"L_2\"maxlength=\"32\"size=\"32\"><br>"));
  client.println(F("<label for=\"Line1\">Line3:</label><input type=\"text\"style=\"font-size:30pt\"id=\"L_3\"name=\"L_3\"maxlength=\"32\"size=\"32\"><br>"));
  client.println(F("<label for=\"Line1\">Line4:</label><input type=\"text\"style=\"font-size:30pt\"id=\"L_4\"name=\"L_4\"maxlength=\"32\"size=\"32\"><br>"));
  client.println(F("<input type=\"submit\"value=\"Set Message\"></form></h1>"));  
  client.println(F("<br><form action=\"http://192.168.1.45\"><input type='submit' value=\"Show Scores\"></form>"));
  client.println(F("</html>"));
}

void servescores(WiFiClient client) {        //Serve full scores page
  showtitle(client);
  showrecent(client);
  showhigh(client);
  showpers(client);
  client.println(F("<br><form action=\"http://192.168.1.45/index.html\"><input type='submit' value=\"New Score\"></form>"));
  client.println(F("</html>"));
}

void serve_inputpage(WiFiClient client, int gm){      //Serve a page for client to enter a high score
    showtitle(client);
    client.println(F("<form action=\"/NewScore\">Machine: <br>"));
    if (gm==0) client.println(F("<input type=\"radio\"style=\"height:30px;width:30px;vertical-align:middle;\"checked id=\"0\"name=\"Machine\"value=\"0\">"));
    else client.println(F("<input type=\"radio\"style=\"height:30px;width:30px;vertical-align:middle;\"id=\"0\"name=\"Machine\"value=\"0\">"));
    client.println(F("<label for=\"0\">Medieval Madness</label><br>"));
    if (gm==1) client.println(F("<input type=\"radio\"style=\"height:30px;width:30px;vertical-align:middle;\"checked id=\"1\"name=\"Machine\"value=\"1\">"));
    else client.println(F("<input type=\"radio\"style=\"height:30px;width:30px;vertical-align:middle;\"id=\"1\"name=\"Machine\"value=\"1\">"));
    client.println(F("<label for=\"1\">Twilight Zone</label><br>"));
    if (gm==2) client.println(F("<input type=\"radio\"style=\"height:30px;width:30px;vertical-align:middle;\"checked id=\"2\"name=\"Machine\"value=\"2\">"));
    else client.println(F("<input type=\"radio\"style=\"height:30px;width:30px;vertical-align:middle;\"id=\"2\"name=\"Machine\"value=\"2\">"));
    client.println(F("<label for=\"2\">Jurassic Park</label><br><br>"));
    client.println(F("<label for=\"Initials\">Initials:</label><input type=\"text\"style=\"font-size:30pt\"id=\"Initials\"name=\"Initials\"maxlength=\"3\"size=\"3\"><br>"));
    client.println(F("<label for=\"Score\">Score:</label><input type=\"number\"style=\"font-size: 30pt\"id=\"Score\"name=\"Score\"min=\"1\"max=\"99999999999\"><br><br>"));
    client.println(F("<input type=\"submit\"value=\"Submit New Score\"></form></h1>"));  
    client.println(F("<form action=\"http://192.168.1.45/ShowScores\"><input type='submit' value=\"Show Scores\"></form>"));
    timesec = usEastern.toLocal(WiFi.getTime());    //Get time from wifi and convert to local time
    sprintf(string,"%d/%d/%d  %02d:%02d:%02d",month(timesec),day(timesec),year(timesec),hour(timesec),minute(timesec),second(timesec));
    client.println(string);
    client.println(F("</html>"));
}

String parsetoken(String line, String beg, String en){  //Function to return substring with begin and end tokens
  int i,j;                                              //Temp variables
  i=line.indexOf(beg)+beg.length();                     //Index of data after begin token
  j=i+line.substring(i).indexOf(en);                    //Index of end token
  return line.substring(i,j);                           //Get the substring
}

void addhighscore(int mach, String in, unsigned long sc, time_t t){ //Function to add a new high score
  int i,j;
  if (nhs[mach]==0) {                                           //Special case - no high scores
    strcpy(hs[0][mach].initials,in.c_str());                    //Insert new initials
    hs[0][mach].score=sc;                                       //Insert new score
    hs[0][mach].t=t;                                            //Insert new time
    nhs[mach]=1;                                                //Set num high scores = 1
  } else if (nhs[mach]<NHS && sc < hs[nhs[mach]-1][mach].score) { //Special case - lower than all other scores and score list not full
    strcpy(hs[nhs[mach]][mach].initials,in.c_str());            //Insert new initials
    hs[nhs[mach]][mach].score=sc;                               //Insert new score
    hs[nhs[mach]][mach].t=t;                                    //Insert new time
    nhs[mach]++;                                                //Increment # high scores
  } else {
    for (i=0;i<nhs[mach];i++) {                                 //Loop over number of high scores for this machine
      if (sc > hs[i][mach].score) {                             //New high score  
        for (j=min(NHS-1,nhs[mach]);j>i;j--) {                  //Loop backwards from last high score
          strcpy(hs[j][mach].initials,hs[j-1][mach].initials);  //Move initials
          hs[j][mach].score=hs[j-1][mach].score;                //Move score
          hs[j][mach].t=hs[j-1][mach].t;                        //Move time
        }
        strcpy(hs[i][mach].initials,in.c_str());                //Insert new initials
        hs[i][mach].score=sc;                                   //Insert new score
        hs[i][mach].t=t;                                        //Insert new time
        if(nhs[mach]<NHS) nhs[mach]++;                          //If less than 10, increment high scores counter
        i=NHS;                                                  //Ends the loop
      }
    }
  }
}

void addpersonalscore(int mach, String in, unsigned long sc, time_t t){ //Function to add new personal score
  int i,nw;
  nw=1;                                   //Set new flag
  for (i=0;i<nps;i++) {                   //Loop over all personal scores 
    if(phs[i].initials == in) {           //If initials match
      if(sc > phs[i].score[mach]){        //If greater than score stored
        phs[i].score[mach]=sc;            //Update the new score
        phs[i].t[mach]=t;                 //Update the new score's time/date
      }
      nw=0;                               //Clear new flag
    }
  }
  if(nw!=0 || nps== 0){                   //If initials were not found or nps =0
    strcpy(phs[nps].initials,in.c_str()); //Copy initials to structure
    for (i=0;i<3;i++){                    //Loop over machines
      phs[nps].score[i]=0;                //Set score to 0
      phs[nps].t[i]=0;                    //Set time/date to 0    
    }
    phs[nps].score[mach]=sc;              //Update the new score
    phs[nps].t[mach]=t;                   //Update the new score's time/date
    nps++;                                //Increment personal score counter
  }
}

void setmessage(String line){                       //Function to set message from client return string
  int i,j;
  strcpy(L[0],parsetoken(line,"L_1=","&").c_str()); //Set first line
  strcpy(L[1],parsetoken(line,"L_2=","&").c_str()); //Set second line
  strcpy(L[2],parsetoken(line,"L_3=","&").c_str()); //Set third line
  strcpy(L[3],parsetoken(line,"L_4="," H").c_str());//Set fourth line
  for (i=0;i<4;i++) {                               //Loop over 4 lines
    for (j=0;j<strlen(L[i]);j++){                   //Loop over each character in the line
      if (L[i][j]=='+') L[i][j]=' ';                //Replace + with space
    }
  }
}

void deletenewscore(String line){                     //Delete one of the new scores - from client return data
  int i,num;
  num=(int)parsetoken(line,"Score=", " ").toInt()-1;  //Get the line to delete
  if (num >= 0 && num < nns) {                        //Only delete if there are scores
    for (i=num;i<nns-1;i++){                          //Loop backwards from last new score to the one to be deleted
      ns[i].machine=ns[i+1].machine;                  //Save the machine
      strcpy(ns[i].initials,ns[i+1].initials);        //Save the initials
      ns[i].score=ns[i+1].score;                      //Save the score
      ns[i].t=ns[i+1].t;                              //Save the time
    }
    nns=nns-1;                                        //Adjust num new scores
    write_EE_scores();                                //Write scores to EEprom
  }
}

void changenewdate(String line){                                          //Change date on a new score - from client return data
  int num;
  time_t t;                                                               //Time
  tmElements_t tmE;                                                       //Structure for creating a time value
  t=usEastern.toLocal(WiFi.getTime());                                    //Get current time - use it for hours, min, sec
  num=(int)parsetoken(line,"Score=", "&").toInt()-1;                      //Get the score to delete
  breakTime(t,tmE);                                                       //Break time into structure
  tmE.Year=(uint8_t)parsetoken(line,"date=","-").toInt()-1970;            //get the year
  tmE.Month=(uint8_t)parsetoken(line,"-","-").toInt();                    //get the month
  tmE.Day=(uint8_t)parsetoken(parsetoken(line,"-","H"),"-"," ").toInt();  //Get the day
  ns[num].t=makeTime(tmE);                                                //Make new time value
  write_EE_scores();                                                      //Write score to EEPROM
}

int gamenum(String gm){                               //Return game number that matches input string
  int g;
  if (gm == "MM") g=0;                                //set game number
  if (gm == "TZ") g=1;                                //set game number
  if (gm == "JP") g=2;                                //set game number
  return g;
}

void deletehighscore(String line){                    //Delete one of the new scores - from client return data
  int i,num,g;
  String gm;
  num=(int)parsetoken(line,"Score=", " ").toInt()-1;  //Get the score to delete
  gm=parsetoken(line,"game="," ");                    //Get the game code
  g=gamenum(gm);
  if (num >= 0 && num < nhs[g]) {                     //Only delete if there are scores
    for (i=num;i<nhs[g]-1;i++){                       //Loop backwards from last new score to the one to be deleted
      hs[i][g].machine=hs[i+1][g].machine;            //Save the machine
      strcpy(hs[i][g].initials,hs[i+1][g].initials);  //Save the initials
      hs[i][g].score=hs[i+1][g].score;                //Save the score
      hs[i][g].t=hs[i+1][g].t;                        //Save the time
    }
    nhs[g]=nhs[g]-1;                                  //Adjust num high scores
    write_EE_scores();                                //Write scores to EEprom
  }
}

void changehighdate(String line){                                         //Change date on a high score - from client return data
  int num,g;
  time_t t;                                                               //Time
  tmElements_t tmE;                                                       //Structure for creating a time value
  t=usEastern.toLocal(WiFi.getTime());                                    //Get current time - use it for hours, min, sec
  num=(int)parsetoken(line,"Score=", "&").toInt()-1;                      //Get the score to delete
  g=gamenum(parsetoken(line,"game=","&"));                                //Get the game code
  breakTime(t,tmE);                                                       //Break time into structure
  tmE.Year=(uint8_t)parsetoken(line,"date=","-").toInt()-1970;            //get the year
  tmE.Month=(uint8_t)parsetoken(line,"-","-").toInt();                    //get the month
  tmE.Day=(uint8_t)parsetoken(parsetoken(line,"-","H"),"-"," ").toInt();  //Get the day
  hs[num][g].t=makeTime(tmE);                                             //Make new time value
  write_EE_scores();                                                      //Write score to EEPROM
}

void changepersdate(String line){                                         //Change date on a personal score - from client return data
  int i,num,g;
  String in;
  time_t t;                                                               //Time
  tmElements_t tmE;                                                       //Structure for creating a time value
  t=usEastern.toLocal(WiFi.getTime());                                    //Get current time - use it for hours, min, sec
  in=parsetoken(line,"initials=","&");                                    //Get the initials 
  for(i=0;i<nps;i++){                                                     //loop over pers score list
    if(in == phs[i].initials) num=i;                                      //If initials match, save index
  }
  g=gamenum(parsetoken(line,"game=","&"));                                //Get the game code
  breakTime(t,tmE);                                                       //Break time into structure
  tmE.Year=(uint8_t)parsetoken(line,"date=","-").toInt()-1970;            //get the year
  tmE.Month=(uint8_t)parsetoken(line,"-","-").toInt();                    //get the month
  tmE.Day=(uint8_t)parsetoken(parsetoken(line,"-","H"),"-"," ").toInt();  //Get the day
  phs[num].t[g]=makeTime(tmE);                                            //Make new time value
  write_EE_scores();                                                      //Write score to EEPROM
}

void deleteperscore(String line){                     //Delete one of the personal high scores - from client return data
  int i,num,g,j;
  String in;
  in=parsetoken(line,"initials=", "&");               //Get the initials 
  g=gamenum(parsetoken(line,"game="," "));            //Get the game number
  for(i=0;i<nps;i++){                                 //loop over pers score list
    if(in == phs[i].initials) num=i;                  //If initials match, save index
  }
  if (phs[num].score[g]>0) {                          //Only delete if there are scores
    phs[num].score[g]=0;                              //Set score to zero
    phs[num].t[g]=0;                                  //set date to zero
    if (phs[num].score[0]==0 && phs[num].score[1]==0 && phs[num].score[2]==0 ) {  //If all scores zero, then delete the row
      for (i=num;i<nps-1;i++){                        //Loop backwards from last new score to the one to be deleted
        strcpy(phs[i].initials,phs[i+1].initials);    //Save the initials
        for (j=0;j<3;j++) {                           //Loop over 3 games
          phs[i].score[j]=phs[i+1].score[j];          //Save the score
          phs[i].t[j]=phs[i+1].t[j];                  //Save the time
        }
      }
      nps=nps-1;                                      //Adjust num pers scores
    }
    write_EE_scores();                                //Write scores to EEprom
  }
}

void addnewscore(int mach, String in, unsigned long sc){  //Add new score, calls add high, add personal, and write EEPROM
  time_t tm;
  int i;
  tm=usEastern.toLocal(WiFi.getTime());       //Get the time
  for (i=nns;i>0;i--) {
    ns[i].machine=ns[i-1].machine;            //Save the machine
    strcpy(ns[i].initials,ns[i-1].initials);  //Save the initials
    ns[i].score=ns[i-1].score;                //Save the score
    ns[i].t=ns[i-1].t;                        //Save the time
  }
  strcpy(ns[0].initials,in.c_str());          //Insert new initials
  ns[0].score=sc;                             //Insert new score
  ns[0].t=tm;                                 //Insert new time
  ns[0].machine=mach;                         //Save machine number
  if(nns<NNS) nns++;                          //If less than 10, increment high scores counter
  addhighscore(mach,in,sc,tm);                //Call high score function
  addpersonalscore(mach,in,sc,tm);            //Call function to add personal scores
  write_EE_scores();                          //Write scores to EEprom
  state=SH_NE;                                //Set state to NE so that it displays new score
  indx=-1;                                    //Set indx to -1
}

void clearscores(String line){                //Function to clear scores - which type is based on return data from client
  if(line.indexOf("ns=1") != -1) nns=0;       //If ns is in the return line from web page, clear nns
  if(line.indexOf("mm=1") != -1) nhs[0]=0;    //If mm is in the return line from web page, clear nhs for mm
  if(line.indexOf("tz=1") != -1) nhs[1]=0;    //If tz is in the return line from web page, clear nhs for tz
  if(line.indexOf("jp=1") != -1) nhs[2]=0;    //If jp is in the return line from web page, clear nhs for jp
  if(line.indexOf("pe=1") != -1) nps=0;       //If pe is in the return line from web page, clear nps
  write_EE_scores();                          //Write scores to EEprom
  get_EE_scores();                            //Read scores from EEprom
}

void serveclear(WiFiClient client){         //Function to serve page with confirmation of clear scores
  showtitle(client);
  client.println(F("<form action=\"/ClearConfirm\">"));
  client.println(F("<legend>Select which score list to clear</legend>"));
  client.println(F("<input type=\"checkbox\" style=\"height:30px;width:30px;\" name=\"ns\" value=1>New Scores<br>"));
  client.println(F("<input type=\"checkbox\" style=\"height:30px;width:30px;\" name=\"mm\" value=1>Medieval Madness<br>"));
  client.println(F("<input type=\"checkbox\" style=\"height:30px;width:30px;\" name=\"tz\" value=1>Twilight Zone<br>"));
  client.println(F("<input type=\"checkbox\" style=\"height:30px;width:30px;\" name=\"jp\" value=1>Jurassic Park<br>"));
  client.println(F("<input type=\"checkbox\" style=\"height:30px;width:30px;\" name=\"pe\" value=1>Personal Scores<br>"));
  client.println(F("<input type=\"submit\"value=\"Yes - Clear\"></form></h1>"));  
  client.println(F("<form action=\"http://192.168.1.45/ShowScores\"><input type='submit' value=\"No - Cancel\"></form></h1>"));
  timesec = usEastern.toLocal(WiFi.getTime());    //Get time from wifi and convert to local time
  sprintf(string,"%d/%d/%d  %02d:%02d:%02d",month(timesec),day(timesec),year(timesec),hour(timesec),minute(timesec),second(timesec));
  client.println(string);
  client.println(F("</html>"));
}

void serve_delnew(WiFiClient client){         //Serve page to allow delete/change new score
  int i;
  time_t t;
  char y[10],m[10],d[10];
  t = usEastern.toLocal(WiFi.getTime());    //Get time from wifi and convert to local time
  showtitle(client);
  client.println(F("<form action=\"/DeleteConfirmNew\">"));
  showrecent(client); //Draw the table of new scores
  client.println(F("<h2>Enter number of which recent score to delete:<br>"));
  client.print(F("<label for=\"Score\">Score Number:</label><input type=\"number\"style=\"font-size: 20pt\"id=\"Score\"name=\"Score\"min=\"1\"max=\""));
  client.print(String(nns));
  client.println(F("\"></h2>"));
  client.println(F("<h1><input type=\"submit\"value=\"Yes - Delete\"></form></h1>"));
  client.println(F("<form action=\"/ChangeNewDate\">"));
  client.println(F("<h2>Enter number of which score to change date:<br>"));
  client.print(F("<label for=\"Score\">Score Number:</label><input type=\"number\"style=\"font-size: 20pt\"id=\"Score\"name=\"Score\"min=\"1\"max=\""));
  client.print(String(nns));
  client.println(F("\">"));
  client.println("New Date:<input type=\"date\" id=\"date\" name=\"date\"style=\"font-size: 20pt\"value=\""+(String)itoa(year(t),y,10)+"-"+(String)itoa(month(t),m,10)+"-"+(String)itoa(day(t),d,10)+"\"</h2>");
  client.println(F("<h1><input type='submit' value=\"Yes - Change Date\"></form></h1>"));
  client.println(F("<h1><form action=\"http://192.168.1.45/ShowScores\"><input type='submit' value=\"No - Cancel\"></form></h1>"));
  client.println(F("</html>"));
}

void serve_delhigh(WiFiClient client){         //Serve page to allow delete/change high score
  int i;
  time_t t;
  char y[10],m[10],d[10];
  t = usEastern.toLocal(WiFi.getTime());    //Get time from wifi and convert to local time
  showtitle(client);
  client.println(F("<form action=\"/DeleteConfirmHigh\">"));
  showhigh(client);
  client.println(F("<h2>Enter number and select game of which score to delete:<br>"));
  client.print(F("<label for=\"Score\">Score Number:</label><input type=\"number\"style=\"font-size: 20pt\"id=\"Score\"name=\"Score\"min=\"1\"max=\""));
  client.print(String(nns));
  client.println(F("\"><br>"));
  client.println(F("Game:<input list=\"game\" name=\"game\"style=\"font-size: 20pt\"style=\"width: 80px;\"></label>"));
  client.println(F("<datalist id=\"game\"><option value=\"MM\"><option value=\"TZ\"><option value=\"JP\"></datalist></h2>"));
  client.println(F("<h1><input type=\"submit\"value=\"Yes - Delete\"></form></h1>"));
  client.println(F("<form action=\"/ChangeHighDate\">"));
  client.println(F("<h2>Enter number and select game of which score to change date:<br>"));
  client.print(F("<label for=\"Score\">Score Number:</label><input type=\"number\"style=\"font-size: 20pt\"id=\"Score\"name=\"Score\"min=\"1\"max=\""));
  client.print(String(nns));
  client.println(F("\"><br>"));
  client.println(F("Game:<input list=\"game\" name=\"game\"style=\"font-size: 20pt\"style=\"width: 80px;\"></label>"));
  client.println(F("<datalist id=\"game\"><option value=\"MM\"><option value=\"TZ\"><option value=\"JP\"></datalist><br>"));
  client.println("New Date:<input type=\"date\" id=\"date\" name=\"date\"style=\"font-size: 20pt\"value=\""+(String)itoa(year(t),y,10)+"-"+(String)itoa(month(t),m,10)+"-"+(String)itoa(day(t),d,10)+"\"</h2>");
  client.println(F("<h1><input type='submit' value=\"Yes - Change Date\"></form></h1>"));
  client.println(F("<h1><form action=\"http://192.168.1.45/ShowScores\"><input type='submit' value=\"No - Cancel\"></form></h1>"));
  client.println(F("</html>"));
}

void serve_delpers(WiFiClient client){    //Serve page to allow delete/change personal score
  int i;
  time_t t;
  char y[10],m[10],d[10];
  t = usEastern.toLocal(WiFi.getTime());    //Get time from wifi and convert to local time
  showtitle(client);
  client.println(F("<form action=\"/DeleteConfirmPers\">"));
  showpers(client);
  client.println(F("<h2>Select initials and select game of which score to delete:<br>"));
  client.println(F("Initials:<input list=\"initials\" name=\"initials\"style=\"font-size: 20pt\"></label>"));
  client.println(F("<datalist id=\"initials\">"));
  for (i=0;i<nps;i++) {
    client.println("<option value=\""+(String)phs[i].initials+"\">");
  }
  client.println(F("</datalist><br>"));
  client.println(F("<br>Game:<input list=\"game\" name=\"game\"style=\"font-size: 20pt\"></label>"));
  client.println(F("<datalist id=\"game\"><option value=\"MM\"><option value=\"TZ\"><option value=\"JP\"></datalist></h2>"));
   client.println(F("<h1><input type=\"submit\"value=\"Yes - Delete\"></form></h1>"));  
  client.println(F("<form action=\"/ChangePersDate\">"));
  client.println(F("<h2>Select initials and select game of which score to change date:<br>"));
  client.println(F("Initials:<input list=\"initials\" name=\"initials\"style=\"font-size: 20pt\"></label>"));
  client.println(F("<datalist id=\"initials\">"));
  for (i=0;i<nps;i++) {
    client.println("<option value=\""+(String)phs[i].initials+"\">");
  }
  client.println(F("</datalist><br>"));
  client.println(F("Game:<input list=\"game\" name=\"game\"style=\"font-size: 20pt\"style=\"width: 80px;\"></label>"));
  client.println(F("<datalist id=\"game\"><option value=\"MM\"><option value=\"TZ\"><option value=\"JP\"></datalist><br>"));
  client.println("New Date:<input type=\"date\" id=\"date\" name=\"date\"style=\"font-size: 20pt\"value=\""+(String)itoa(year(t),y,10)+"-"+(String)itoa(month(t),m,10)+"-"+(String)itoa(day(t),d,10)+"\"</h2>");
  client.println(F("<h1><input type='submit' value=\"Yes - Change Date\"></form></h1>"));
  client.println(F("<h1><form action=\"http://192.168.1.45/ShowScores\"><input type='submit' value=\"No - Cancel\"></form></h1>"));
  client.println(F("</html>"));
}

void parseget(String line, WiFiClient client){                    //Parse the client GET input line, take appropriate action, and serve next page
  int machine,i;
  unsigned long score;
  String temp,initials;
  if(line.indexOf("NewScore") != -1) {                            //Check if the Get is a new score
    machine=(int)parsetoken(line,"Machine=", "&").toInt();        //Get the machine int
    initials=parsetoken(line,"Initials=", "&");                   //Get the initials
    for (i=0;i<3;i++){                                            //Loop over 3 initials
      initials[i]=toupper(initials[i]);                           //Force to uppercase
    }
    score=(unsigned long)parsetoken(line,"Score=", " ").toInt();  //Get the score
    addnewscore(machine, initials, score);                        //Add the score to the lists
    servescores(client);                                          //When done serve the input page
  } else if (line.indexOf("ShowScores") != -1){                   //If request is to show scores
    servescores(client);                                          //Show scores
  } else if (line.indexOf("ClearScores") != -1){                  //If request is to clear scores
    serveclear(client);                                           //serve page with confirmation
  } else if (line.indexOf("ClearConfirm") != -1){                 //If request to clear is confirmed
    clearscores(line);                                            //clear scores
    servescores(client);                                          //Show scores
  } else if (line.indexOf("index.html") != -1){                   //If index.html
    serve_inputpage(client,0);                                    //So serve the input page
   } else if (line.indexOf("Admin") != -1){                       //If Admin
    serve_adminpage(client);                                      //So serve the Admin page
   } else if (line.indexOf("/Message") != -1){                    //If set message
    setmessage(line);                                             //Set the message
    servescores(client);                                          //Show scores
   } else if (line.indexOf("MM_New") != -1){                      //If MM
    serve_inputpage(client,0);                                    //So serve the input page
  } else if (line.indexOf("TZ_New") != -1){                       //If TZ
    serve_inputpage(client,1);                                    //So serve the input page
  } else if (line.indexOf("JP_New") != -1){                       //If JP
    serve_inputpage(client,2);                                    //So serve the input page
  } else if (line.indexOf("DelNew") != -1) {                      //Delete new score
    serve_delnew(client);                                         //Serve the delete page
  } else if (line.indexOf("DeleteConfirmNew") != -1){             //If request to delete is confirmed
    deletenewscore(line);                                         //delete score
    servescores(client);                                          //Show scores
  } else if (line.indexOf("ChangeNewDate") != -1){                //If request to change date is confirmed
    changenewdate(line);                                          //change date
    servescores(client);                                          //Show scores
  } else if (line.indexOf("DelHigh") != -1) {                     //Delete new score
    serve_delhigh(client);                                        //Serve the delete page
  } else if (line.indexOf("DeleteConfirmHigh") != -1){            //If request to delete is confirmed
    deletehighscore(line);                                        //delete score
    servescores(client);                                          //Show scores
  } else if (line.indexOf("ChangeHighDate") != -1){               //If request to change date is confirmed
    changehighdate(line);                                         //change date
    servescores(client);                                          //Show scores
  } else if (line.indexOf("DelPers") != -1) {                     //Delete new score
    serve_delpers(client);                                        //Serve the delete page
  } else if (line.indexOf("DeleteConfirmPers") != -1){            //If request to delete is confirmed
    deleteperscore(line);                                         //delete score
    servescores(client);                                          //Show scores
  } else if (line.indexOf("ChangePersDate") != -1){               //If request to change date is confirmed
    changepersdate(line);                                         //change date
    servescores(client);                                          //Show scores
  } else {                                                        //If no other match
    servescores(client);                                          //Show scores
  }
}

char wserver[] = "api.openweathermap.org"; 

char* dir(int angle){                         //Return string of wind direction
  float fangle;
  fangle = (float)angle + 11.25;              // Offset angle to be able to find heading
  if (fangle > 360.0) fangle -= 360.0;        //fix rollover if required  
  return(heading[(int)(fangle/22.5)]);        //Return code from array
}

void getWeather() { 
  WiFiClient client;
  char temp[100];
  String s;
  float f;
  int i;
  if (Serial) Serial.println("\nStarting connection to server..."); 
 // if you get a connection, report back via serial: 
  if (client.connect(wserver, 80)) { 
   if (Serial) Serial.println("connected to server"); 
   // Make a HTTP request: 
   client.print("GET /data/2.5/weather?"); 
   client.print("q=Ann Arbor,US"); 
   client.print("&appid=c7e87f81186ea78be08a2dfdd7eec70a"); 
   client.println("&units=imperial"); 
   client.println("Host: api.openweathermap.org"); 
   client.println("Connection: close"); 
   client.println(); 
 } else { 
   if (Serial) Serial.println("unable to connect"); 
 }
  delay(1000); 
  String line = ""; 
  while (client.connected()) { 
    line = client.readStringUntil('\n'); 
    wea.temp=parsetoken(line, "main\":{\"temp\":", ",").toFloat();
    wea.feels=parsetoken(line, "\"feels_like\":", ",").toFloat();
    wea.wsp=parsetoken(line, "\"wind\":{\"speed\":", ",").toFloat();
    wea.wang=parsetoken(line, "\"deg\":", ",").toInt();
    wea.hum=parsetoken(line, "\"humidity\":", ",").toInt();
    wea.sunrise=usEastern.toLocal(parsetoken(line, "\"sunrise\":", ",").toInt());
    wea.sunset=usEastern.toLocal(parsetoken(line, "\"sunset\":", ",").toInt());
    wea.cond=parsetoken(line, "\"description\":\"", "\"");
    wea.code=parsetoken(line, "icon\":\"", "\"").toInt();
    if (Serial) Serial.println(line); 
  } 
} 

void service_wifi(){                                          //Check if new wifi data and process it
  WiFiClient client = server.available();                     //Check if server is running
  String line = "";                                           //Clear the string used for retrieving characters 
  if (client) {                                               //If it is running
    while (client.connected()) {                              //While client is connected
      if (client.available()) {                               //If there is a character available to read
        char c = client.read();                               //read the character from client buffer
        if (Serial) Serial.write(c);                          //Echo the character to serial momitor
        if (c == '\n') {                                      //Check for end of line character
          if(line.length()==0) {                              //If blank line that means end of HTTP message
            break;                                            //Break the while
          } else {                                            //Else the completed line is not emptuy
            if(line.startsWith("GET"))parseget(line,client);  //Check if the line is a "Get" - and if so parse it
            line="";                                          //Clear the string so ready for next line
          }
        }else if (c != '\r') {                                //If the character is not return
          line+=c;                                            //Add received character to the input string
        }
      }
    }
    client.stop();                                            //Stop client after it is done processing
    if (Serial) Serial.println("Client stopped");             //Debug info
  }
}

void trans(char* line, int num){              //Transmit data to display via I2C
  Wire.beginTransmission(6);                  //transmit to device #6
  if (num>0) Wire.write(line,num);            //Send line - fixed length
  else Wire.write(line);                      //send line - not fixed length
  Wire.endTransmission(1);                     //stop transmitting
}

void sendcolor(int color){                                      //Send color
  sprintf(line,"%c,%c%c%c",SETTEXTCOLOR,color/256,color%256,0); //Set Color
  trans(line,4);                                                //Transmit command to display
}

void sendcursor(int x, int y){                    //Send cursor position
  sprintf(line,"%c,%c,%c%c",SETCURSOR,x,y,0);     //Set cursor
  trans(line,5);                                  //Transmit command to display
}

void sendtext(char *text){                        //Send a text string
  sprintf(line,"%c,",PRINTLN);                    //Show score number
  trans(strcat(line,text),0);                     //Transmit to display
}

void sendclrln(int ln){                           //Send clear line. 0-4 clears single line. 8 clears line 1,2,3. 9 clears 0,1,2,3
  sprintf(line,"%c,%c%c",CLEARLN,ln,0);           //Clear line ln
  trans(line,3);                                  //Transmit command to display
}

void sendshow(){                                   //Send clear line
  sprintf(line,"%c%c",SHOW,0);                     //Show
  trans(line,2);                                   //Transmit command to display
}

void sendcurscoltext(int x, int y, int color, char *text){  //Send the coords, color and text all together
  sprintf(line,"%c,%c%c%c%c",CURSCOLTEXT,x,y,color/256,color%256);  //Create first part of line - code, coords, color
  strcpy(&line[6],text);                                            //Add text starting at 7th byte
  trans(line,strlen(text)+6);                                       //Transmit to display, have to use length
}

void writenew(int i, int index, int j){                                 //Write one line of new score
  int y;
  char text[40];                                                        //Temp needed to convert score to string
  y=((i+1)*8)-j;                                                        //Calculate y coordinate for text
  sendcurscoltext(0, y, BLUE, itoa(index+1,text,10));                   //Send Index #
  sendcurscoltext(18, y, MC[ns[index].machine], MI[ns[index].machine]); //Send Machine code
  sendcurscoltext(36, y, WHITE, ns[index].initials);                    //Send Initials
  sendcurscoltext(60, y, WHITE, scorestring(ns[index].score));          //Send score with commas
  sendcurscoltext(144, y, WHITE, date2(ns[index].t));                   //Send date
}

void writehigh(int i, int index, int gm, int j){                    //Write one line of high score
  int y;
  char text[40];                                                    //Temp needed to convert score to string
  y=((i+1)*8)-j;                                                    //Set y value for this line
  sendcurscoltext(3, y, BLUE, itoa(index+1,text,10));               //Send score number
  sendcurscoltext(25, y, WHITE, hs[index][gm].initials);            //Send Inititals
  sendcurscoltext(53, y, WHITE, scorestring(hs[index][gm].score));  //Send score with commas
  sendcurscoltext(141, y, WHITE, date2(hs[index][gm].t));           //Send date
}

void writepers(int i, int index){                                     //Write one line of personal high score
  int y;
  y=(i+1)*8;                                                          //Set Y coord for this line
  sendcurscoltext(5,y, WHITE, phs[index].initials);                   //Send Inititals
  sendcurscoltext(29,y, MC[i], MI[i]);                                //Send machine name 
  if(phs[index].score[i] != 0) {                                      //Only erite a score if it is not zero
    sendcurscoltext(53, y, WHITE, scorestring(phs[index].score[i]));  //Send score with commas
    sendcurscoltext(143, y, WHITE, date2(phs[index].t[i]));           //Send date
  }
}

void writenewentry(){                                               //Draws new score screen 
  char tempstr[80];
  sendcurscoltext((192-18*6)/2,0, ORANGE, "New Score Entered:");    //Send a title line
  strcpy(tempstr,Machine[ns[0].machine].c_str());                   //Copy the string to char*
  sendcurscoltext(xcenter(tempstr),12, MC[ns[0].machine], tempstr); //Send machine name
  strcpy(tempstr,ns[0].initials);                                   //Initials
  strcat(tempstr,": ");                                             //Separator
  strcat(tempstr,scorestring(ns[0].score));                         //Add score
  sendcurscoltext(xcenter(tempstr),24,WHITE,tempstr);               //Send the initials and new score
}

void writemess(){                                   //Draws message screen
  int i,j;
  int y,y0,ydel;
  j=0;                                              //Counter of how many lines
  for (i=0;i<4;i++) {                               //Loop over lines
    if (strlen(L[i])>0) j++;                        //Check if any characters, if so increment line counter
  }
  y0=(32-8*j)/(j+1);                                //Y coordinate of first line
  ydel=y0+8;                                        //Delta for next lines
  y=y0;
  for (i=0;i<4;i++){                                //Loop over the lines
    if (strlen(L[i])>0) {                           //If it has characters
      sendcursor(xcenter(L[i]),y);                  //Send cursor position - to be centered
      sendtext(L[i]);                               //Send a line
      if (Serial) Serial.println(L[i]);
      y+=ydel;                                      //Increment Y coordinate for next line
    }
  }
}    

int tempcolor(float temp){            //Returns color for temperature
  if (temp < 32) return BLUE;
  else if (temp < 50) return WHITE;
  else if (temp < 80) return GREEN;
  else if (temp < 90) return ORANGE;
  else return RED;
}

int condcolor(int code){                //Return color for weather condition
  if(code ==1) return GREEN;            //Clear
  else if (code <= 4) return GREY;      //Clouds
  else if (code <= 10) return ORANGE;   //Rain
  else if (code == 11) return RED;      //Thunderstorm
  else if (code == 13) return CYAN;     //Snow
  else if (code == 50) return YELLOW;   //Mist
}

int wscolor(float ws){                                  //Return color for windspeed
  if (ws <=5) return GREEN;
  else if (ws <= 15) return YELLOW;
  else if (ws <= 25) return ORANGE;
  else return RED;
}

void writewea(int i, int index, int j){                                 //Write one line of weather. i=line number to draw on, indx = line of weather
  int y;
  char text[40];                                                        //Temp needed to convert score to string
  y=((i+1)*8)-j;                                                        //Calculate y coordinate for text
  switch (index) {
    case 1:                                                                             //Temp
      sendcurscoltext(0, y, WHITE, "Temp:");                                            //Send label
      sendcurscoltext(6*6, y, tempcolor(wea.temp),(char*)String(wea.temp,1).c_str());   //Send temperature - with color coded value
      sendcurscoltext(13*6, y, WHITE, "Feels Like:");                                   //Send label
      sendcurscoltext(25*6,y,tempcolor(wea.feels),(char*)String(wea.feels,1).c_str());  //Send temperature - with color coded value
    break;
    case 2:                                                                             //Wind/hum
      sendcurscoltext(0, y, WHITE, "Wind:");                                            //Send label
      sendcurscoltext(6*6, y, wscolor(wea.wsp),(char*)String(wea.wsp,1).c_str());       //Send wind speed
      sendcurscoltext(11*6, y, WHITE, "mph");                                           //Send mph
      sendcurscoltext(15*6, y, WHITE, dir(wea.wang));                                   //Send wind direction
      sendcurscoltext(19*6, y, WHITE, "Humidity:");                                     //Send label
      sendcurscoltext(29*6, y, YELLOW, (char*)String(wea.hum).c_str());                   //Send wind speed
      sendcurscoltext(31*6, y, WHITE, "%");                                             //Send % character
     break;
    case 0:                                                                             //Conditions
      sendcurscoltext(0, y, WHITE, "Cond: ");                                           //Send label
      sendcurscoltext(6*6, y, condcolor(wea.code), (char*)wea.cond.c_str());            //Send condition
    break;
    case 3:                                                                             //Sunrise/sunset
      sendcurscoltext(0, y, WHITE, "Sunrise:");                                         //Send label
      sprintf(text,"%d:%02d ",hourFormat12(wea.sunrise),minute(wea.sunrise));           //Construct time string
      sendcurscoltext(9*6, y, WHITE, text);                                             //Send string
      sendcurscoltext(15*6, y, WHITE, "Sunset:");                                       //Send label
      sprintf(text,"%d:%02d ",hourFormat12(wea.sunset),minute(wea.sunset));             //Construct time string
      sendcurscoltext(23*6, y, WHITE, text);                                            //Send string
    break;
  }
}

void title(char *text, int col, int x, int y){  //Draws title line given text, color, coords
  sendclrln(0);                                 //Clear first line
  sendcurscoltext(x, y, col, text);             //Send cursor, color, and text
}

#define SPLASHDEL 5000      //Show title screen for 5 seconds
#define FSDEL 3000          //Hold first score 3 sec before starting scrolling
#define SCOREDEL 300        //Scroll delay for new and high scores
#define PHSDEL 5000         //show personal scores for 5 seconds each
#define NEDEL 10000         //Show new score for 10 seconds
#define MEDEL 8000          //Show message for 8 seconds
#define WUPDEL 60000        //Get weather update every 60 seconds
time_t wlast;
int nwl=4;                  //Number of weather lines

void loop() {                                           //3 items in this loop: process wifi, process state, drive display
  int i,j;                                              //Temporary variable
  char tempstr[80],datestr[20];                         //Temp strings for display purpose
  time_t tm;                                            //Temp variable for time function
  service_wifi();                                       //Service the wifi
  t_now=millis();                                       //Get current time
  if (t_now > wlast+WUPDEL && state==SH_SP) {           //Get weather data only during splash screen so it does not delay proper scrolling
    getWeather();
    wlast = t_now;
  }
  switch(state) {                                       //Swithc on display state
    case SH_SP:                                         //Show splash screen
      if (indx == -1) {
        sendclrln(9);                                   //Clear all 4 display lines
        title("Richard's Pinball Scoreboard",BLUE,xcenter("Richard's Pinball Scoreboard"),0);//Create title
        tm=usEastern.toLocal(WiFi.getTime());           //Get the time
        strcpy(tempstr,dayStr(weekday(tm)));            //Day of week
        strcat(tempstr," ");                            //Add a space
        strcat(tempstr,monthStr(month(tm)));            //Write the month
        sprintf(datestr," %d, %d",day(tm),year(tm));    //Create string for day and year
        strcat(tempstr,datestr);                        //Add to weekday and month
        sendcurscoltext(xcenter(tempstr),12,WHITE,tempstr);  //Send the date
        sprintf(tempstr,"%d:%02d ",hourFormat12(tm),minute(tm));    //Construct time string
        if (isAM(tm)) strcat(tempstr,"AM");             //If AM append text
        else strcat(tempstr,"PM");                      //Else append PM text
        sendcurscoltext(xcenter(tempstr),24,WHITE,tempstr);  //Send the time
        sendshow();                                     //Update the display
        indx++;                                         //Increment index so not redraw splash
        t_last = t_now;                                 //Save the last time
      }
      if (t_now > (t_last+SPLASHDEL)) {                 //Check if splash timer has elapsed
        state = SH_WE;                                  //Next state is show weather
        indx=0;                                         //Start at first new score
        scroll=0;                                       //Clear the scroll counter
        t_last = t_now;                                 //reset last time
      }
    break;    
    case SH_WE:
      if (t_now > (t_last+SCOREDEL)) {                  //If ready to scroll to next score
        t_last = t_now;                                 //Save the last time
        if (indx >= nwl) {                              //If at last weather line
          state = SH_NS;                                //Set next state to high scores
          indx=0;                                       //Clear the indx
          scroll=0;                                     //Clear the scroll counter
          break;
        } else {
          sendclrln(8);                                 //Clear the lower 3 lines
          for (i=0;i<4;i++) {                           //Loop over 3 display lines, 4(partial) if scrolling
            if (indx+i < nns) {                         //If this line has data
              writewea(i,indx+i,scroll);                //Write the data
            }
          }
          title("Current Weather:",ORANGE,xcenter("Current Weather:"),0);   //Create title
          if (indx==0 && scroll==0) t_last+=FSDEL;      //Adjust time to keep first score fully visible for a second before starting scrolling
          sendshow();                                   //Done with 3 lines so refresh screen
        }
        if(scroll<7)scroll++;                           //Increment scroll counter if less than one full line
        else {                                          //Otherwise
          scroll=0;                                     //Clear the scroll counter
          indx++;                                       //Increment index
        }
      }
    
    break;
    case SH_NS:                                         //Show new score list
      if (t_now > (t_last+SCOREDEL)) {                  //If ready to scroll to next score
        t_last = t_now;                                 //Save the last time
        if (indx >= nns) {                              //If at last new score
          state = SH_HS;                                //Set next state to high scores
          indx=0;                                       //Clear the indx
          scroll=0;                                     //Clear the scroll counter
          gm=0;                                         //Set to first game
          break;
        } else {
          sendclrln(8);                                 //Clear the lower 3 lines
          for (i=0;i<4;i++) {                           //Loop over 3 display lines, 4(partial) if scrolling
            if (indx+i < nns) {                         //If this line has data
              writenew(i,indx+i,scroll);                //Write the data
            }
          }
          title("Recent Pinball Scores",ORANGE,xcenter("Recent Pinball Scores"),0);   //Create title
          if (indx==0 && scroll==0) t_last+=FSDEL;      //Adjust time to keep first score fully visible for a second before starting scrolling
          sendshow();                                   //Done with 3 lines so refresh screen
        }
        if(scroll<7)scroll++;                           //Increment scroll counter if less than one full line
        else {                                          //Otherwise
          scroll=0;                                     //Clear the scroll counter
          indx++;                                       //Increment index
        }
      }
    break;    
    case SH_HS:                                         //Show high score list
      if (t_now > (t_last+SCOREDEL)) {                  //If time to scroll
        t_last = t_now;                                 //Save the last time
        sendclrln(8);                                   //Clear the lower 3 lines
        for (i=0;i<(scroll==0?3:4);i++) {               //Loop over 3 display lines, 4(partial) if scrolling
           if (indx+i < nhs[gm]) {                      //If there is a score to display
            writehigh(i,indx+i,gm,scroll);              //Write the high score line
          }
        }
        title("High Scores - ",ORANGE,6,0);             //Create title
        strcpy(tempstr,Machine[gm].c_str());            //Copy the string to char*
        sendcurscoltext(90, 0, MC[gm], tempstr);        //Send Inititals
        if (indx==0 && scroll==0) t_last+=FSDEL;        //Adjust time to keep first score fully visible for a second before starting scrolling
        sendshow();                                     //Done so refresh screen
        if(scroll<7)scroll++;                           //Increment scroll counter if less than one full line
        else {                                          //Otherwise
          scroll=0;                                     //Clear scroll counter
          indx++;                                       //Increment index
        }
        if (indx >= nhs[gm]) {                          //If at last new score for this game
          if (gm == 2) {                                //If on last game
            state = SH_PS;                              //Go to next state
            t_last -= (PHSDEL);                         //Adjust time so that personal scores show right away
            indx=-1;                                    //Clear the indez
          } else {                                      //Not at last game
            indx=0;                                     //Set index to zero
            scroll=0;                                   //Clear the scroll counter
            gm++;                                       //Increment game counter
          }
        }
      }
    break;    
    case SH_PS:                                         //Show personal score list
      if (t_now > (t_last+PHSDEL)) {                    //If ready to scroll to next person
        if (indx == -1){                                //First time in this state - clear screem and put up title
          title("Personal High Scores",ORANGE,xcenter("Personal High Scores"),0);    //Create title
          indx=0;
        } 
        sendclrln(8);                                   //Clear the lower 3 lines
        for (i=0;i<3;i++) {                             //Loop over 3 display lines
          writepers(i,indx);                            //Write the data
        }
        sendshow();                                     //Done with 3 lines so refresh screen
        t_last = t_now;                                 //Save the last time
        indx++;                                         //Increment index
        if (indx >= nps) {                              //If at last personal score
          j=0;                                          //Clear the variable
          for(i=0;i<4;i++) j+=strlen(L[i]);             //Check if any message characters
          if (j>0) state = SH_ME;                       //if any, then show message
          else state = SH_SP;                           //Else Set next state to Splash
          indx=-1;                                      //indx == -1 says to draw title
          delay(PHSDEL);                                //Keep showing last display for its time
          t_last = t_now;                               //Save the last time
        } 
      }      
    break;
    case SH_ME:
      if (indx == -1) {                                   //If the first time for Message
        sendclrln(9);                                     //Clear all 4 display lines
        writemess();                                      //Write the message
        indx++;                                           //Increment
        t_last = t_now;                                   //Save the time
        sendshow();                                       //Done with 3 lines so refresh screen
      }
      if (t_now > (t_last + MEDEL)) {                     //Check if time for next state
        state=SH_SP;                                      //Go to new score list
        indx=-1;                                          //Set indx to print new title
        t_last = t_now;                                   //Save the last time
      }
    break;
    case SH_NE:                                           //Show new entry
      if (indx == -1) {
        sendclrln(9);                                     //Clear all 4 display lines
        writenewentry();                                  //Draw new entry screen
        playfanfare();                                    //Play sound
        sendshow();                                       //refresh screen
        indx++;                                           //Increment
        t_last = t_now;                                   //Save the time
      }
      if (t_now > (t_last + NEDEL)) {                     //Check if time for next state
        state=SH_NS;                                      //Go to new score list
        indx=0;                                           //Set indx to print new title
        scroll=0;
        t_last = t_now - SCOREDEL;                        //Adjust last time so that it immediately shows new scores
      }
    break;
  }
//  sprintf(string,"State=%d, last=%lu, now=%lu, indx=%d, state=%d\n",state,t_last,t_now,indx,state);
//  if (Serial) Serial.print(string);
//  delay(10);
}  
  

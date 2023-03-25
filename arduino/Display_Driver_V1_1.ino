
// Display Driver program for 3x 64x32 LED matrix
//Copyright RJH January 2022
#include <Wire.h>                                   //Needed for I2C comm from master controller
#include <Adafruit_Protomatter.h>                   //LED matrix library

// Color definitions
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define GREY    (BLUE/16 + RED/16 + GREEN/16)
#define ORANGE (matrix.color565(255,64,0))
//Display connections - These are for MRK1010
uint8_t rgbPins[]  = {A3, A4, A5, A6, 2, 3};
uint8_t addrPins[] = {4, 5, 6, 7};
uint8_t clockPin   = A0;
uint8_t latchPin   = 13;
uint8_t oePin      = 14; 

Adafruit_Protomatter matrix(192, 2, 1, rgbPins, 4, addrPins, clockPin, latchPin, oePin, false); //Instantiate the matrix

void setup(void) {                                          //Setup function
  Serial.begin(9600);                                       //Serial port for debug
  ProtomatterStatus status = matrix.begin();                // Initialize matrix...
  if (Serial) Serial.print("Protomatter begin() status: ");  
  if (Serial) Serial.println((int)status);                  //Report initialization status of matrix
  if(status != PROTOMATTER_OK) {                            //If not OK then just wait until it is
    for(;;);
  }
  matrix.fillRect(0,0,192,32,BLACK);                        //Clear the display
  matrix.setTextColor(WHITE);                               //Set color
  matrix.setCursor(10,0);                                   //Set cursor
  matrix.println("Display is Initializing");                //Print startup message
  matrix.setCursor(10,12);                                  //Set cursor
  matrix.println("Please Wait");                            //Remainder of startup message
  matrix.show(); // Copy data to matrix buffers             //Must show after write to buffer  
  delay(1000);                                              //Wait 1 second before starting I2C
  Wire.begin(6);                                            //Join i2c bus with address #6 - this is where display data comes from
  Wire.onReceive(receiveEvent);                             //Callback for new I2C data
}

char line[80];                                              //Global variable used for displaying text
//Defines for the different display commands - must match the sender's definition
#define SETCURSOR 1 
#define SETTEXTCOLOR 2
#define PRINTLN 3
#define SHOW 4
#define CLEARLN 5
#define CURSCOLTEXT 6

void receiveEvent(int howMany){                             //Callback for the I2C communication
  int index=0;                                              //Temp vafiable 
  int x,y;                                                  //X&Y variables for setting cursor
  long int col;                                             //Variable for setting color
  while(0 < Wire.available()) line[index++] = Wire.read();  //Receive byte as a character
  line[index]=0;                                            //Add null to end of the string
  if (Serial) Serial.println(line);                         //If serial port, print debug info
  switch (line[0]) {                                        //First character is operation type
    case SETCURSOR:                                         //first char is operation, then comma, then x, then comma, then y, then \n
      x=(int)line[2];                                       //Get x value
      y=(int)line[4];                                       //Get y value
      matrix.setCursor(x,y);                                //Set cursor
    break;
    case SETTEXTCOLOR:                                      //First char is operation type
      col=(int)line[3]+256*(int)line[2];                    //2 bytes for color
      matrix.setTextColor(col);                             //Set the color
    break;   
    case PRINTLN:                                           //First char is operation type
      matrix.println(&line[2]);                             //Print the text to the display buffer
    break;  
    case SHOW:                                              //First char is operation type
      matrix.show();                                        //Copy data to matrix buffers
    break;  
    case CLEARLN:                                           //First char is operation type
      x=(int)line[2];                                       //Get which line to clear
      if(x<4) matrix.fillRect(0,x*8,192,x*8+8,BLACK);       //Clear a line
      else if(x==8) matrix.fillRect(0,1*8,192,3*8+8,BLACK); //Clear lines 2,3,4
      else if(x==9) matrix.fillRect(0,0*8,192,3*8+8,BLACK); //Clear lines 1,2,3,4
    break;  
    case CURSCOLTEXT:                                       //first char is operation, then comma, then x, then y, then 2 bytes color then text
      x=(int)line[2];                                       //Get x value
      y=(int)line[3];                                       //Get y value
      matrix.setCursor(x,y);                                //Set cursor
      col=(int)line[5]+256*(int)line[4];                    //2 bytes for color
      matrix.setTextColor(col);                             //Set the color
      matrix.println(&line[6]);                              //Print the text
    break;
  }
}

void loop(void) {                                           //Loop function has no real work
  if (Serial) Serial.print("Refresh FPS = ~");              //Print diagnostic info
  if (Serial) Serial.println(matrix.getFrameCount());       //Display update frame rate
  delay(1000);                                              //Wait 1 second
}

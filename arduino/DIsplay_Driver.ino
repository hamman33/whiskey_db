#include <Wire.h>
#include <Adafruit_Protomatter.h>

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
//These are for MRK1010
uint8_t rgbPins[]  = {A3, A4, A5, A6, 2, 3};
uint8_t addrPins[] = {4, 5, 6, 7};
uint8_t clockPin   = A0;
uint8_t latchPin   = 13;
uint8_t oePin      = 14; 

Adafruit_Protomatter matrix(192, 2, 1, rgbPins, 4, addrPins, clockPin, latchPin, oePin, true);

void setup(void) {
  Serial.begin(9600);
  int16_t x, y;
  uint16_t w, h;

  ProtomatterStatus status = matrix.begin();        // Initialize matrix...
  Serial.print("Protomatter begin() status: ");
  Serial.println((int)status);
  if(status != PROTOMATTER_OK) {
    for(;;);
  }
//    matrix.drawPixel(x, matrix.height() - 4, matrix.color565(level, 0, 0));


//  matrix.getTextBounds("Pinball Scoreboard", 0, 0, &x, &y, &w, &h);  
  matrix.setCursor(10,0);
  matrix.setTextColor(ORANGE); //Orange = 255,64,0
  matrix.println("High Scores - Jurassic Park"); // Default text color is white
  matrix.setTextColor(WHITE); 
  matrix.setCursor(3,8);
  matrix.println("1");
  matrix.setCursor(25,8);
  matrix.println("LCH");
  matrix.setCursor(53,8);
  matrix.println("1,367,278,387");
  matrix.setCursor(141,8);
  matrix.println("12/28/21");
  matrix.setCursor(3,16);
  matrix.println("2");
  matrix.setCursor(25,16);
  matrix.println("RJH");
  matrix.setCursor(53,16);
  matrix.println("  245,234,390");
  matrix.setCursor(141,16);
  matrix.println("12/28/22"); 

/*  matrix.setCursor(10,0);
  matrix.setTextColor(ORANGE); //Orange = 255,64,0
  matrix.println("Recent Scores"); // Default text color is white
  matrix.setTextColor(WHITE); 
  matrix.setCursor(0,8);
  matrix.println("1");
  matrix.setCursor(18,8);
  matrix.println("MM");  
  matrix.setCursor(36,8);
  matrix.println("LCH");
  matrix.setCursor(60,8);
  matrix.println("  367,278,387");
  matrix.setCursor(144,8);
  matrix.println("12/28/21");
  matrix.setCursor(0,16);
  matrix.println("10");
  matrix.setCursor(18,16);
  matrix.println("TZ");  
  matrix.setCursor(36,16);
  matrix.println("RJH");
  matrix.setCursor(60,16);
  matrix.println("1,245,234,390");
  matrix.setCursor(144,16);
  matrix.println("12/28/21");*/
  matrix.show(); // Copy data to matrix buffers
  Wire.begin(4);                // join i2c bus with address #4
  Wire.onReceive(receiveEvent); // register event
  Serial.begin(9600);           // start serial for output
}
char line[80];
#define SETCURSOR 1
#define SETTEXTCOLOR 2
#define PRINTLN 3
#define SHOW 4
#define CLEARLN 5

void receiveEvent(int howMany){
  int index=0;
  int x,y;
  long int col;
  while(0 < Wire.available()) line[index++] = Wire.read(); // receive byte as a character
  Serial.print(line);
  switch (line[0]) {                      //First character is operation type
    case SETCURSOR:                       //first char is operation, then comma, then x, then comma, then y, then \n
      x=(int)line[2];                     //Get x value
      y=(int)line[4];                     //Get y value
      matrix.setCursor(x,y);              //Set cursor
    break;
    case SETTEXTCOLOR:                    //First char is operation type
      col=(int)line[3]+256*(int)line[2];  //2 bytes for color
      matrix.setTextColor(col);           //Set the color
    break;   
    case PRINTLN:                         //First char is operation type
      matrix.println(line[2]);            //Print the text
    break;  
    case SHOW:                            //First char is operation type
      matrix.show();                      //Copy data to matrix buffers
    break;  
    case CLEARLN:                         //First char is operation type
      x=(int)line[2];                     //Get which line
      matrix.fillRect(0,x*8,191,x*8+7,BLACK);   //Clear a line
    break;  
  }
}

void loop(void) {
  Serial.print("Refresh FPS = ~");
  Serial.println(matrix.getFrameCount());
  delay(1000);
}

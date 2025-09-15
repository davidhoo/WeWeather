#include <Arduino.h>
#include <SPI.h>

//IO settings
int BUSY_Pin = D1; 
int RES_Pin = D4; 
int DC_Pin = D2; 
int CS_Pin = D8; 

// Pin manipulation macros
#define EPD_W21_CS_0 digitalWrite(CS_Pin,LOW)
#define EPD_W21_CS_1 digitalWrite(CS_Pin,HIGH)
#define EPD_W21_DC_0  digitalWrite(DC_Pin,LOW)
#define EPD_W21_DC_1  digitalWrite(DC_Pin,HIGH)
#define EPD_W21_RST_0 digitalWrite(RES_Pin,LOW)
#define EPD_W21_RST_1 digitalWrite(RES_Pin,HIGH)
#define isEPD_W21_BUSY digitalRead(BUSY_Pin)

// Display dimensions
#define EPD_WIDTH 128
#define EPD_HEIGHT 296

// Display buffer
unsigned char displayBuffer[EPD_WIDTH * EPD_HEIGHT / 8];

////////FUNCTION//////
void driver_delay_us(unsigned int xus);
void driver_delay_xms(unsigned long xms);
void DELAY_S(unsigned int delaytime);     
void SPI_Delay(unsigned char xrate);

//EPD
void Epaper_READBUSY(void);
void SPI_Write(unsigned char value);
void Epaper_Write_Command(unsigned char cmd);
void Epaper_Write_Data(unsigned char datas);

void EPD_HW_Init(void); //Electronic paper initialization
void EPD_Update(void);
void EPD_WhiteScreen_ALL_Clean(void);
void EPD_DeepSleep(void);

//Display 
void EPD_WhiteScreen_ALL(const unsigned char *BW_datas);
void EPD_SetFullWindow(void);
void EPD_SetPartWindow(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1);
void EPD_Part_Update(void);

// Drawing functions
void clearDisplay(void);
void drawText(const char* text, int x, int y);
void displayBufferToScreen(void);
void displayPartBufferToScreen(int x, int y, int width, int height);

void Sys_run(void)
{
   ESP.wdtFeed(); //Feed dog to prevent system reset
}

void LED_run(void)
{
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
  delay(500);
  digitalWrite(LED_BUILTIN, HIGH);   // Turn the LED on (Note that LOW is the voltage level
  delay(500);
}

void setup() {
   pinMode(BUSY_Pin, INPUT); 
   pinMode(RES_Pin, OUTPUT);  
   pinMode(DC_Pin, OUTPUT);    
   pinMode(CS_Pin, OUTPUT);    
   pinMode(LED_BUILTIN, OUTPUT);
   
   //SPI
   SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0)); 
   SPI.begin ();  
   
   // Initialize display
   EPD_HW_Init(); //EPD init
   EPD_WhiteScreen_ALL_Clean();
   EPD_DeepSleep();//EPD_DeepSleep,Sleep instruction is necessary, please do not delete!!!
   delay(2000); //2s  
   
   // Wake up and clear screen
   EPD_HW_Init(); //EPD init
   clearDisplay();
   displayBufferToScreen();
   EPD_Update();
   
   // Wait 2 seconds
   delay(2000);
   
   // Draw "Testing..." at top-left corner
   drawText("Testing...", 0, 20);
   displayBufferToScreen();
   EPD_Update();
}

////////Partial refresh schematic////////////////

/////Y/// (0,0)        /---/(x,y)
          //                 /---/
          //                /---/  
          //x
          //
          //
//Tips//
/*
1. When refreshing the electronic paper in full screen, the picture flickers is a normal phenomenon, the main function is to clear the residual image displayed in the previous picture.
2. When performing partial refresh, the screen will not flicker. It is recommended to use full-screen refresh to clear the screen after 5 partial refreshes to reduce screen image retention.
3. After the e-paper is refreshed, it needs to enter the sleep mode, please do not delete the sleep command.
4. Do not remove the e-paper when turning on.
5. Wake up from sleep, need to re-initialize the electronic paper.
6. When you need to transplant the driver, you only need to change the corresponding IO. The BUSY pin is in input mode, and the other pins are in output mode.
*/

int counter = 0;

void loop() {
  // Wait 10 seconds
  delay(10000);
  
  // Increase counter
  counter++;
  
  // Clear the area where counter will be displayed (128x50 area at position 0,100)
  // Fill with white (0xFF)
  for(int y = 100; y < 150; y++) {
    for(int x = 0; x < 128; x++) {
      int byteIndex = (y * EPD_WIDTH + x) / 8;
      int bitIndex = 7 - (x % 8);
      displayBuffer[byteIndex] |= (1 << bitIndex); // Set bit to 1 (white)
    }
  }
  
  // Convert counter to string
  char counterStr[12];  // Increased buffer size to fix warning
  sprintf(counterStr, "%d", counter);
  
  // Draw counter in the center of the area (approximate)
  drawText(counterStr, 50, 120);
  
  // Display partial area (0,100) to (127,149)
  EPD_SetPartWindow(0, 100, 127, 149);
  displayPartBufferToScreen(0, 100, 128, 50);
  EPD_Part_Update();
  
  Sys_run();//System run
  //LED_run();//Breathing lamp
}

///////////////////EXTERNAL FUNCTION////////////////////////////////////////////////////////////////////////
void SPI_Write(unsigned char value)                                    
{                                                           
    SPI.transfer(value);
}

void Epaper_Write_Command(unsigned char cmd)
{
  EPD_W21_CS_0;                   
  EPD_W21_DC_0;   // command write
  SPI_Write(cmd);
  EPD_W21_CS_1;
}

void Epaper_Write_Data(unsigned char datas)
{
  EPD_W21_CS_0;                   
  EPD_W21_DC_1;   //data write
  SPI_Write(datas);
  EPD_W21_CS_1;
}

/////////////////EPD settings Functions/////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
void EPD_HW_Init(void)
{
  EPD_W21_RST_0;  // Module reset      
  delay(10); //At least 10ms delay 
  EPD_W21_RST_1; 
  delay(10); //At least 10ms delay 
  
  Epaper_READBUSY();   
  Epaper_Write_Command(0x12);  //SWRESET
  Epaper_READBUSY();   
    
  Epaper_Write_Command(0x01); //Driver output control      
  Epaper_Write_Data(0x27);
  Epaper_Write_Data(0x01);
  Epaper_Write_Data(0x00);

  Epaper_Write_Command(0x11); //data entry mode       
  Epaper_Write_Data(0x01);

  Epaper_Write_Command(0x44); //set Ram-X address start/end position   
  Epaper_Write_Data(0x00);
  Epaper_Write_Data(0x0F);    //0x0F-->(15+1)*8=128

  Epaper_Write_Command(0x45); //set Ram-Y address start/end position          
  Epaper_Write_Data(0x27);   //0x0127-->(295+1)=296
  Epaper_Write_Data(0x01);
  Epaper_Write_Data(0x00);
  Epaper_Write_Data(0x00); 

  Epaper_Write_Command(0x3C); //BorderWavefrom
  Epaper_Write_Data(0x05); 
   
  Epaper_Write_Command(0x18); //Read built-in temperature sensor
  Epaper_Write_Data(0x80);  
  
  Epaper_Write_Command(0x21); //  Display update control
  Epaper_Write_Data(0x00);    
  Epaper_Write_Data(0x80);          
     
  Epaper_Write_Command(0x4E);   // set RAM x address count to 0;
  Epaper_Write_Data(0x00);
  Epaper_Write_Command(0x4F);   // set RAM y address count to 0X199;    
  Epaper_Write_Data(0x27);
  Epaper_Write_Data(0x01);
  Epaper_READBUSY();
}

//////////////////////////////All screen update////////////////////////////////////////////
void EPD_WhiteScreen_ALL(const unsigned char *BW_datas)
{
   unsigned int i;
    Epaper_Write_Command(0x24);   //write RAM for black(0)/white (1)
   for(i=0;i<EPD_WIDTH * EPD_HEIGHT / 8;i++)
   {               
     Epaper_Write_Data(BW_datas[i]);
   }
   EPD_Update();   
}

/////////////////////////////////////////////////////////////////////////////////////////
void EPD_Update(void)
{   
  Epaper_Write_Command(0x22); //Display Update Control
  Epaper_Write_Data(0xF7);   
  Epaper_Write_Command(0x20); //Activate Display Update Sequence
  Epaper_READBUSY();  
}

void EPD_Part_Update(void)
{
  Epaper_Write_Command(0x22);//Display Update Control 
  Epaper_Write_Data(0xFF);   
  Epaper_Write_Command(0x20); //Activate Display Update Sequence
  Epaper_READBUSY();      
}

void EPD_DeepSleep(void)
{  
  Epaper_Write_Command(0x10); //enter deep sleep
  Epaper_Write_Data(0x01); 
  delay(100);
}

void Epaper_READBUSY(void)
{ 
  while(1)
  {   //=1 BUSY
     if(isEPD_W21_BUSY==0) break;
     ESP.wdtFeed(); //Feed dog to prevent system reset
  }  
}

/////////////////////////////////Single display////////////////////////////////////////////////
void EPD_WhiteScreen_ALL_Clean(void)
{
   unsigned int i;
      Epaper_Write_Command(0x24);   //write RAM for black(0)/white (1)
   for(i=0;i<EPD_WIDTH * EPD_HEIGHT / 8;i++)
   {               
     Epaper_Write_Data(0xff);
   }

   EPD_Update();   
}

void EPD_SetFullWindow(void)
{
  Epaper_Write_Command(0x44); //set Ram-X address start/end position   
  Epaper_Write_Data(0x00);
  Epaper_Write_Data(0x0F);    //0x0F-->(15+1)*8=128

  Epaper_Write_Command(0x45); //set Ram-Y address start/end position          
  Epaper_Write_Data(0x27);   //0x0127-->(295+1)=296
  Epaper_Write_Data(0x01);
  Epaper_Write_Data(0x00);
  Epaper_Write_Data(0x00); 
}

void EPD_SetPartWindow(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1)
{
  Epaper_Write_Command(0x44); //set Ram-X address start/end position   
  Epaper_Write_Data(x0/8);
  Epaper_Write_Data(x1/8);    

  Epaper_Write_Command(0x45); //set Ram-Y address start/end position          
  Epaper_Write_Data(y0%256);
  Epaper_Write_Data(y0/256);
  Epaper_Write_Data(y1%256);
  Epaper_Write_Data(y1/256); 
}

// Drawing functions
void clearDisplay(void)
{
  // Fill buffer with white (0xFF)
  for(int i = 0; i < EPD_WIDTH * EPD_HEIGHT / 8; i++) {
    displayBuffer[i] = 0xFF;
  }
}

void drawText(const char* text, int x, int y)
{
  // Simple character drawing - this is a basic implementation
  // For a complete implementation, you would need to include font data
  // This is just a placeholder to show the concept
  for(int i = 0; text[i] != '\0'; i++) {
    // Draw character at position (x + i*8, y)
    // This is a simplified implementation that just draws a box for each character
    for(int dy = 0; dy < 8 && (y + dy) < EPD_HEIGHT; dy++) {
      for(int dx = 0; dx < 8 && (x + i*8 + dx) < EPD_WIDTH; dx++) {
        int pixelX = x + i*8 + dx;
        int pixelY = y + dy;
        int byteIndex = (pixelY * EPD_WIDTH + pixelX) / 8;
        int bitIndex = 7 - (pixelX % 8);
        
        // Set bit to 0 (black) - simplified representation
        displayBuffer[byteIndex] &= ~(1 << bitIndex);
      }
    }
  }
}

void displayBufferToScreen(void)
{
  Epaper_Write_Command(0x24);   //write RAM for black(0)/white (1)
  for(int i = 0; i < EPD_WIDTH * EPD_HEIGHT / 8; i++) {
    Epaper_Write_Data(displayBuffer[i]);
  }
}

void displayPartBufferToScreen(int x, int y, int width, int height)
{
  Epaper_Write_Command(0x24);   //write RAM for black(0)/white (1)
  
  // Calculate start and end bytes for the partial area
  int startByte = (y * EPD_WIDTH + x) / 8;
  int endByte = ((y + height) * EPD_WIDTH + (x + width) - 1) / 8;
  
  // Write only the partial data
  for(int i = startByte; i <= endByte; i++) {
    Epaper_Write_Data(displayBuffer[i]);
  }
}
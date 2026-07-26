#include <Arduino.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <JPEGDEC.h>

#define TJPGD_LOAD_SD_LIBRARY

#define UP 27
#define DW 26
#define OK 25
#define BK 33
#define VISIBLE_ITEMS 6

JPEGDEC jpg;
File fileHandle;
TFT_eSPI tft = TFT_eSPI();       // Invoke custom library

int JPEGDraw(JPEGDRAW* pDraw);
void* myOpen(const char* filename, int32_t* size);
void myClose(void *handle);
int32_t myRead(JPEGFILE* handle, uint8_t *buffer, int32_t length);
int32_t mySeek(JPEGFILE* handle, int32_t pos);

int loopingFile();
void loadVisible();
void drawMenuItem(int visibleIndex);
void drawMenu();


int screen = 0;


struct DirectoryState{
  int totalFiles = 0; // total file found in SD
  int selectedIndex = 0; // Selected File index (0 to totalFiles -1)
  int topIndex = 0; // Index of the top-most visible item on screen

}nav;


// Buffer to store only the currently visible items on screen
char visibleName[VISIBLE_ITEMS][32];


void setup(){

  Serial.begin(115200);

  pinMode(UP,INPUT);
  pinMode(DW,INPUT);
  pinMode(OK, INPUT);
  pinMode(BK, INPUT);

  tft.begin();

  if(!SD.begin(5, tft.getSPIinstance())){
    Serial.println("Card Mount Failed");
    return;
  }

  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No Card attached");
    return;
  }

  Serial.print("SD Card Type: ");

  if(cardType == CARD_MMC){
    Serial.println("MMC");
  }else if(cardType == CARD_SD){
    Serial.println("SDSC");
  }else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  }else{
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024*1024);
  Serial.printf("Sd Card size: %lluMB\n",cardSize);
  Serial.println("Initialization Done");

  tft.fillScreen(TFT_BLACK);

  nav.totalFiles = loopingFile(); // Getting Total Number of jpg file

  loadVisible(); // loading the visible item from menu
  drawMenu(); // drawing them

}


void loop(){

  if(digitalRead(UP) == LOW && screen == 0){
      
    if(nav.selectedIndex > 0){
      
      int oldTop = nav.topIndex;
      int oldSelection = nav.selectedIndex;
      
      nav.selectedIndex--;

      if(nav.selectedIndex < nav.topIndex){
        nav.topIndex = nav.selectedIndex;
        loadVisible();

        for(int i = 0; i < VISIBLE_ITEMS; i++){
            drawMenuItem(i);
          }
      }else{
        drawMenuItem(oldSelection - oldTop);
        drawMenuItem(nav.selectedIndex - nav.topIndex);
      }
    
    }
    delay(200);
    while(digitalRead(UP) == LOW);
  }


  if(digitalRead(DW) == LOW && screen == 0){
    if(nav.selectedIndex < nav.totalFiles - 1){
      
      int oldTop = nav.topIndex;
      int oldSelection = nav.selectedIndex;

      nav.selectedIndex++;
      /*
      If selectedIndex is greater then or equal to 6 which is the last item, in the display We
      need to increment the topindex so we can show the 7th index in the display. for finding the top
      index we just simply subtract the visible items = 6 from the selected index and add 1. 
      For example, selected index = 7, then topIndex = 7-6+1 = 2. then we call the loadVisible and draw
      menu.
      */
      if(nav.selectedIndex >=  nav.topIndex+VISIBLE_ITEMS){
        nav.topIndex = nav.selectedIndex - VISIBLE_ITEMS + 1;
        loadVisible();
        
        for (int i = 0; i < VISIBLE_ITEMS; i++) {
          drawMenuItem(i);
        }

      }else {
         // Only selection changed
        drawMenuItem(oldSelection - oldTop); // Clearing the old selection. Here nav.selected index will be false
        drawMenuItem(nav.selectedIndex - nav.topIndex); //  Here nav.selectedIndex will be true
      }
    }
    delay(200);
    while(digitalRead(DW) == LOW);
  }

  if(digitalRead(OK) == LOW && screen == 0){


    if(nav.totalFiles > 0){
      tft.fillScreen(TFT_BLACK);
      int localIndex = nav.selectedIndex - nav.topIndex; // It converts the global selected index into a local/visible index.
      char* selectedPath = visibleName[localIndex];

      if(jpg.open((char*)selectedPath,myOpen,myClose,myRead,mySeek,(JPEG_DRAW_CALLBACK*)JPEGDraw)){
        jpg.setPixelType(RGB565_BIG_ENDIAN);

        jpg.decode(0,0,0);
        jpg.close();
        screen = 1;
      }else{
        Serial.printf("Failed to open JPG: ");
        Serial.println(selectedPath);
        tft.fillScreen(TFT_BLACK);
        drawMenu();
        screen = 0;
      }

    }

    delay(200);
    while(digitalRead(OK) == LOW);
  }


    if(digitalRead(BK) == LOW && screen != 0 ){
    
    screen = 0;
    tft.fillScreen(TFT_BLACK);
    drawMenu();
    delay(200);
    while(digitalRead(BK) == LOW);
  }

}

void* myOpen(const char* filename, int32_t* size){

  fileHandle = SD.open(filename);
  if (fileHandle) {
    *size = fileHandle.size();
    return &fileHandle;
  }
  return NULL;
}

void myClose(void *handle){
  if(fileHandle){fileHandle.close();}

}

int32_t myRead(JPEGFILE* handle, uint8_t *buffer, int32_t length){
  if(!fileHandle){return 0;}

  return fileHandle.read(buffer, length);
}

int32_t mySeek(JPEGFILE* handle, int32_t pos){
  if(!fileHandle){return 0;}

  return fileHandle.seek(pos);
}


int loopingFile(){

  int fileCount = 0;
  File dir = SD.open("/");
  if(!dir) return 0;

  while(true){
    File entry = dir.openNextFile();

    if(!entry){break;}
    
    if(entry.isDirectory() == false){
      const char* name = entry.name();
      const int len = strlen(name);

      if(len >=4  && strcasecmp(name+len-4,".jpg") == 0){
        //Serial.print("File: ");
       // Serial.println(name);
        
        fileCount++;

      }      

      
    }

    entry.close();
  }

  dir.close();

  return fileCount;

}


// Fetch Only Visible items

void loadVisible(){
  File dir = SD.open("/");
  if(!dir) return;

  int jpgMatchCount = 0; // How many JPG files have I seen so far?
  int loadedCount = 0;

  memset(visibleName,0,sizeof(visibleName)); // Clear the visible name array

  while(loadedCount < VISIBLE_ITEMS){
    File entry = dir.openNextFile();
    if(!entry) break;

    if(entry.isDirectory() == false){
      const char* name = entry.name();
      const int len = strlen(name);

      if(len >= 4 && (strcasecmp(name+len-4,".jpg") == 0) || strcasecmp(name+len-4,".jpeg")){
        if(jpgMatchCount >= nav.topIndex) // Increment increase from nav.TopIndex adhering to top Index.
          {

          if(name[0] == '/'){
            snprintf(visibleName[loadedCount],sizeof(visibleName[loadedCount]),"%s",name);
          }else{
            snprintf(visibleName[loadedCount],sizeof(visibleName[loadedCount]),"/%s",name);
          }
          loadedCount++;
        }
        jpgMatchCount++;
      }
    }
    entry.close();
  }
  dir.close();
}


void drawMenuItem(int visibleIndex){

  if(visibleIndex < 0 || visibleIndex >= VISIBLE_ITEMS){
    return;
  }

  int x = 10;
  int fontNum = 4;
  int height = tft.fontHeight(fontNum);
  int w = 150;
  int y = 10 + (height + 10)*visibleIndex;

  int itemGlobalIndex = nav.topIndex+visibleIndex; // This converts the screen position into the actual file index.
  if(itemGlobalIndex >= nav.totalFiles) return;
  
  bool isSelected = (itemGlobalIndex == nav.selectedIndex);


    if(isSelected == true){
        tft.fillRect(x-5,y-5,w+4,height+4, TFT_NAVY);
        tft.setTextColor(TFT_WHITE, TFT_NAVY); // (foreground, background)

    }else{
        tft.fillRect(x-5,y-5,w+4,height+4, TFT_BLACK);

        tft.drawRect(x-5,y-5,w+4,height+4, TFT_NAVY);
        tft.setTextColor(TFT_WHITE, TFT_BLACK); // (foreground, background)

    }
    tft.setTextDatum(TL_DATUM);
    tft.drawString(visibleName[visibleIndex],x,y,fontNum);

}


void drawMenu(){
  tft.fillScreen(TFT_BLACK);

  if(nav.totalFiles == 0){
    tft.drawString("NO Jpg or JPEG file found", 20,50,4);
    return;
  }

  loadVisible();

  for(int i = 0; i < VISIBLE_ITEMS; i++){
    drawMenuItem(i);
    
  }

}


int JPEGDraw(JPEGDRAW* pDraw)
{

  tft.pushImage(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);
  return 1;
}
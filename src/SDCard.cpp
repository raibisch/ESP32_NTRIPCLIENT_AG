#include <Arduino.h>


#include "global_vars.h"


#ifdef USE_WEMOSBOARD
#include <SSD1306.h> // alias for `#include "SSD1306Wire.h"'
#endif

#ifdef USE_M5STICKC
#include <M5StickC.h>
#endif

#ifdef USE_M5STACK
#include <M5Stack.h>                                         /* Include M5 Stack library */
#endif

#include <SD_MMC.h>

bool sd_init()
{ 
  //SPIFFS.begin();
  if (!SD.begin(TFCARD_CS_PIN, SPI,400000U))
  {
    Serial.println("SD_MMC-Card: initialization failed.");
    return false;
  } else 
  {
    Serial.println("SD_MMC-Card: Wiring is correct and a card is present.");
  }
 
  // print the type of card
  Serial.println();
  Serial.print("Card type:         ");
  switch (SD.cardType()) {
    case CARD_NONE:
      Serial.println("NONE");
      break;
    case CARD_MMC:
      Serial.println("MMC");
      break;
    case CARD_SD:
      Serial.println("SD");
      break;
    case CARD_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }
  
  return true;
}

bool sd_readfile(fs::FS &fs, const char * path) 
{
    Serial.printf("Reading file: %s\n", path);
    //M5.Lcd.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file)
    {
        Serial.println("Failed to open file for reading");
        return false;
    }

    Serial.print("Read from file: ");
    while(file.available())
    {
        int ch = file.read();
        Serial.write(ch);
    }
    file.close();
    return true;
}

bool sd_writefile(fs::FS &fs, const char * path, String s){
    Serial.printf("Writing file: %s\n", path);
    //M5.Lcd.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("ERR: Failed to open file for writing");
        return false;
    }

    if(file.print(s))
    {
        Serial.println("OK: File written");
        //M5.Lcd.println("File written");
    } 
    else 
    {
        Serial.println("Write failed");
        file.close();
        return false;
    }
    file.close();
    return true;
}

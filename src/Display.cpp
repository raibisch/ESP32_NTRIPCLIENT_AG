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



#ifdef USE_WEMOSBOARD
static SSD1306  oled(0x3c, 5, 4);
#endif


void display_clear()
{
#ifdef USE_WEMOSBOARD
  oled.clear();
#endif

#ifdef USE_M5STACK
  M5.Lcd.fillScreen(TFT_BLACK);
#endif 
}


void display_init()
{
#ifdef USE_WEMOSBOARD
  oled.init();
  oled.flipScreenVertically();
  oled.setFont(ArialMT_Plain_16);
  oled.setTextAlignment(TEXT_ALIGN_LEFT);
#endif

#ifdef USE_M5STACK
  M5.begin();
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextSize(3);
  delay(1);
#endif
}

void display_text(byte line_nr, String s, uint32_t color)
{
   #ifdef USE_WEMOSBOARD
  //oled.drawString(0, line_nr*17, "_____________");
  //oled.display();
  
  oled.drawString(0, line_nr*16, s);
  //oled.display();
#endif
#ifdef USE_M5STACK 
    M5.Lcd.fillRect(0, line_nr*33, 320, 33, TFT_BLACK);
    M5.Lcd.setCursor(5, line_nr*33+5);
    delay(1);
    M5.Lcd.setTextColor(color);
    M5.Lcd.print(s);
    delay(1);
#endif
}


void display_text(byte line_nr, String s)
{
  display_text(line_nr, s, TFT_WHITE);
}


void display_status(String s, char status)
{
  if (s.length() > 0)
  {
   if (status == 'E')
    M5.Lcd.fillRect(0,6*33,320,33, TFT_RED);
   else
    M5.Lcd.fillRect(0,6*33,320,33, TFT_DARKGREEN);
    
   M5.Lcd.setCursor(5, 6*33+5);
   yield();
   M5.Lcd.print(s);
   yield();
  }
  else
  {
   M5.Lcd.fillRect(0,6*33,320,33, TFT_BLACK);
  }
}


void display_error(String s)
{
   display_status(s, 'E');
}

void display_info(String s)
{
  display_status(s, 'I');
}

void display_display()
{
#ifdef USE_WEMOSBOARD
  oled.display();
  delay(1);
#endif
}
